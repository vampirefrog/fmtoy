#include <stdio.h>
#include <string.h>

#include "tools.h"

struct vgm_file {
	uint32_t version;
	uint32_t eof_offset;
	uint32_t gd3_offset;
	uint32_t data_offset;

	uint32_t ym2413_clock;
	uint32_t ym2612_clock;
	uint32_t ym2151_clock;
	uint32_t ym2203_clock;
	uint32_t ym2608_clock;
	uint32_t ym2610_clock;
	uint32_t ym3812_clock;
	uint32_t ym3526_clock;
	unsigned
		ym2413_dual:1,
		ym2151_dual:1,
		ym2612_dual:1,
		ym2203_dual:1,
		ym2608_dual:1,
		ym2610_dual:1,
		ym3812_dual:1,
		ym3526_dual:1;
};

#define READ_UINT32(data, x) (data[x] | (data[x + 1] << 8) | (data[x + 2] << 16) | (data[x + 3] << 24))
#define READ_OFFSET(field, data, ofs) { field = READ_UINT32(data, ofs); if(field != 0) field += ofs; }
int vgm_file_load(struct vgm_file *f, uint8_t *data, size_t data_len) {
	if(data_len < 0x40) return -1;

	memset(f, 0, sizeof(*f));

	f->version = READ_UINT32(data, 0x08);
	READ_OFFSET(f->eof_offset, data, 0x04);
	READ_OFFSET(f->data_offset, data, 0x34);
	READ_OFFSET(f->gd3_offset, data, 0x14);
#define READ_CHIP_CLOCK(chip, ofs, ver) \
	if(f->version >= ver) { \
		f->chip##_clock = READ_UINT32(data, ofs); \
		f->chip##_dual = f->chip##_clock & 0x40000000 ? 1 : 0; \
		f->chip##_clock &= 0x3fffffff; \
	} else { \
		f->chip##_clock = 0; \
		f->chip##_dual = 0; \
	}

	READ_CHIP_CLOCK(ym2413, 0x10, 0x100);
	READ_CHIP_CLOCK(ym2612, 0x2c, 0x110);
	READ_CHIP_CLOCK(ym2151, 0x30, 0x110);
	READ_CHIP_CLOCK(ym2203, 0x44, 0x151);
	READ_CHIP_CLOCK(ym2608, 0x48, 0x151);
	READ_CHIP_CLOCK(ym2610, 0x4c, 0x151);
	READ_CHIP_CLOCK(ym3812, 0x50, 0x151);
	READ_CHIP_CLOCK(ym3526, 0x54, 0x151);

	return 0;
}

// get the size of a VGM command, or -1 if it runs out of bytes
int vgm_cmd_size(uint8_t *bytes, int remaining_bytes) {
	if(remaining_bytes < 1) return -1;
	if(bytes[0] == 0x67) {
		if(remaining_bytes >= 8) {
			int dataLen = 7 + ((bytes[6] << 24) | (bytes[5] << 16) | (bytes[4] << 8) | bytes[3]);
			return (dataLen >= remaining_bytes) ? -1 : dataLen;
		} else return -1;
	} else if((bytes[0] >= 0x62 && bytes[0] <= 0x63) || (bytes[0] >= 0x70 && bytes[0] <= 0x8f)) return 1;
	else if((bytes[0] >= 0x30 && bytes[0] <= 0x3f) || (bytes[0] >= 0x4f && bytes[0] <= 0x50)) return 2;
	else if((bytes[0] >= 0x40 && bytes[0] <= 0x4e) || (bytes[0] >= 0x51 && bytes[0] <= 0x5f) || (bytes[0] >= 0xa0 && bytes[0] <= 0xbf)) return 3;
	else if(bytes[0] >= 0xc0 && bytes[0] <= 0xdf) return 4;
	else if((bytes[0] >= 0x90 && bytes[0] <= 0x91) || (bytes[0] >= 0xe0 && bytes[0] <= 0xff)) return 5;
	else switch(bytes[0]) {
		case 0x66: return 1;
		case 0x94: return 2;
		case 0x61: return 3;
		case 0x95: return 5;
		case 0x92: return 6;
		case 0x93: return 11;
		case 0x68: return 12;
	}

	return -1;
}

struct opn_voice_operator {
	uint8_t dt_mul;
	uint8_t tl;
	uint8_t ks_ar;
	uint8_t am_dr;
	uint8_t sr;
	uint8_t sl_rr;
	uint8_t ssg_eg;
};

struct opn_voice {
	uint8_t fb_connect;
	int vgm_ofs;
	uint8_t chan_used_mask;
	struct opn_voice_operator operators[4];
};

struct opn_voice *opn_voices = 0;
int num_opn_voices;

void opn_voice_dump(struct opn_voice *v) {
	printf("fb=%d connect=%d\n", v->fb_connect >> 3 & 0x07, v->fb_connect & 0x07);
	for(int i = 0; i < 4; i++) {
		struct opn_voice_operator *op = &v->operators[i];
		printf("operator = %d\n", i+1);
		printf("dt=%d multi=%d\n", op->dt_mul >> 4 & 0x07, op->dt_mul & 0x0f);
		printf("tl=%d\n", op->tl);
		printf("ks=%d ar=%d\n", op->ks_ar >> 6 & 0x03, op->ks_ar & 0x1f);
		printf("am=%d dr=%d\n", op->am_dr >> 7, op->am_dr & 0x1f);
		printf("sr=%d\n", op->sr & 0x1f);
		printf("sl=%d rr=%d\n", op->sl_rr >> 4, op->sl_rr & 0x0f);
		printf("ssg_eg=%d\n", op->ssg_eg & 0x0f);
	}
}

void opn_voice_dump_opm(struct opn_voice *v, int n) {
	printf(
		"// vgm offset = %08x, channels used = %c%c%c%c%c%c%c%c\n",
		v->vgm_ofs,
		v->chan_used_mask & 0x01 ? '1' : '-',
		v->chan_used_mask & 0x02 ? '2' : '-',
		v->chan_used_mask & 0x04 ? '3' : '-',
		v->chan_used_mask & 0x08 ? '4' : '-',
		v->chan_used_mask & 0x10 ? '5' : '-',
		v->chan_used_mask & 0x20 ? '6' : '-',
		v->chan_used_mask & 0x40 ? '7' : '-',
		v->chan_used_mask & 0x80 ? '8' : '-'
	);
	printf("@:%d Instrument %d\n", n, n);
	printf("LFO: 0 0 0 0 0\n");
	printf("CH: 64 %i %i 0 0 120 0\n", v->fb_connect >> 3 & 0x07, v->fb_connect & 0x07);
	int op_order[4] = { 0, 2, 1, 3 };
	char *op_names[4] = { "M1", "C1", "M2", "C2" };
	uint8_t detune_values[8] = { 3, 4, 5, 6, 3, 2, 1, 0 };
	for(int i = 0; i < 4; i++) {
		struct opn_voice_operator *op = &v->operators[op_order[i]];
		printf("%s: %2d %2d %2d %2d %2d %3d %2d %2d %2d 0 0\n",
			op_names[i],
			op->ks_ar & 0x1f,    // AR -> AR
			op->am_dr & 0x1f,    // DR -> D1R
			op->sr & 0x1f,       // SR -> D2R
			op->sl_rr & 0x0f,    // RR -> RR
			op->sl_rr >> 4,      // SL -> D1L
			op->tl & 0x7f,       // TL -> TL
			op->ks_ar >> 6,      // KS -> KS
			op->dt_mul & 0x0f, // MULTI -> MUL
			detune_values[op->dt_mul >> 4 & 0x07] // DT -> DT1
		);
	}
	printf("\n");
}

void push_opn_voice(uint8_t *regs, uint8_t port, uint8_t chan, uint8_t mask, int vgm_ofs) {
	uint8_t *ofs = regs + chan + port * 256;

	struct opn_voice voice;
	voice.fb_connect = ofs[0xb0];
	voice.vgm_ofs = vgm_ofs;
	for(int i = 0; i < 4; i++) {
		voice.operators[i].dt_mul   = ofs[0x30 + i * 4];
		voice.operators[i].tl       = ofs[0x40 + i * 4];
		voice.operators[i].ks_ar    = ofs[0x50 + i * 4];
		voice.operators[i].am_dr    = ofs[0x60 + i * 4];
		voice.operators[i].sr       = ofs[0x70 + i * 4];
		voice.operators[i].sl_rr    = ofs[0x80 + i * 4];
		voice.operators[i].ssg_eg   = ofs[0x90 + i * 4];
	}

	uint8_t slot_masks[8] = { 0x08,0x08,0x08,0x08,0x0c,0x0e,0x0e,0x0f };
	int existing_voice = -1;
	for(int i = 0; i < num_opn_voices; i++) {
		struct opn_voice *v = &opn_voices[i];
		if(v->fb_connect != voice.fb_connect) continue;
		uint8_t slot_mask = slot_masks[voice.fb_connect & 0x07];
		int good = 1;
		for(int j = 0, m = 1; j < 4; j++, m <<= 1) {
			struct opn_voice_operator *o1 = &v->operators[j];
			struct opn_voice_operator *o2 = &voice.operators[j];
			if(
				o1->dt_mul != o2->dt_mul ||
				o1->ks_ar != o2->ks_ar ||
				o1->am_dr != o2->am_dr ||
				o1->sr != o2->sr ||
				o1->sl_rr != o2->sl_rr
			) {
				good = 0;
				break;
			}
			if(!(slot_mask & m)) {
				if(o1->tl != o2->tl) {
					good = 0;
					break;
				}
			}
		}
		if(!good) continue;

		existing_voice = i;
		break;
	}
	if(existing_voice < 0) {
		existing_voice = num_opn_voices++;
		opn_voices = realloc(opn_voices, num_opn_voices * sizeof(struct opn_voice));
		if(!opn_voices) {
			fprintf(stderr, "Could not reallocate %d OPN voices\n", num_opn_voices);
			return;
		}
		memcpy(&opn_voices[existing_voice], &voice, sizeof(voice));
	} else {
		struct opn_voice *v = &opn_voices[existing_voice];
		v->chan_used_mask |= 1 << (chan + port * 4);
		uint8_t slot_mask = slot_masks[v->fb_connect & 0x07];
		// find lowest attenuation, or highest volume
		for(int i = 0, m = 1; i < 4; i++, m <<= 1) {
			if(slot_mask & m) {
				if(voice.operators[i].tl < v->operators[i].tl)
					v->operators[i].tl = voice.operators[i].tl;
			}
		}
	}
}

struct opm_voice_operator {
	uint8_t dt1_mul;
	uint8_t tl;
	uint8_t ks_ar;
	uint8_t ame_d1r;
	uint8_t dt2_d2r;
	uint8_t d1l_rr;
};

struct opm_voice {
	uint8_t fb_connect;
	int vgm_ofs;
	uint8_t chan_used_mask;
	struct opm_voice_operator operators[4];
};

struct opm_voice *opm_voices = 0;
int num_opm_voices;

void opm_voice_dump_opm(struct opm_voice *v, int n) {
	printf(
		"// vgm offset = %08x, channels used = %c%c%c%c%c%c%c%c\n",
		v->vgm_ofs,
		v->chan_used_mask & 0x01 ? '1' : '-',
		v->chan_used_mask & 0x02 ? '2' : '-',
		v->chan_used_mask & 0x04 ? '3' : '-',
		v->chan_used_mask & 0x08 ? '4' : '-',
		v->chan_used_mask & 0x10 ? '5' : '-',
		v->chan_used_mask & 0x20 ? '6' : '-',
		v->chan_used_mask & 0x40 ? '7' : '-',
		v->chan_used_mask & 0x80 ? '8' : '-'
	);
	printf("@:%d Instrument %d\n", n, n);
	printf("LFO: 0 0 0 0 0\n");
	printf("CH: 64 %i %i 0 0 120 0\n", v->fb_connect >> 3 & 0x07, v->fb_connect & 0x07);
	int op_order[4] = { 0, 2, 1, 3 };
	char *op_names[4] = { "M1", "C1", "M2", "C2" };
	for(int i = 0; i < 4; i++) {
		struct opm_voice_operator *op = &v->operators[op_order[i]];
		printf("%s: %2d %2d %2d %2d %2d %3d %2d %2d %2d %2d %2d\n",
			op_names[i],
			op->ks_ar & 0x1f,   // AR
			op->ame_d1r & 0x1f, // D1R
			op->dt2_d2r & 0x1f, // D2R
			op->d1l_rr & 0x0f,  // RR
			op->d1l_rr >> 4,    // D1L
			op->tl & 0x7f,      // TL
			op->ks_ar >> 6,     // KS
			op->dt1_mul & 0x0f, // MUL
			op->dt1_mul >> 4,   // DT1
			op->dt2_d2r >> 6,   // DT2
			op->ame_d1r >> 7    // AME
		);
	}
	printf("\n");
}

void push_opm_voice(uint8_t *regs, uint8_t port, uint8_t chan, uint8_t mask, int vgm_ofs) {
	uint8_t *ofs = regs + chan + port * 256;

	struct opm_voice voice;
	voice.fb_connect = ofs[0x20];
	voice.vgm_ofs = vgm_ofs;
	for(int i = 0; i < 4; i++) {
//		printf("%d %d dt1_mul=0x%02x\n", chan, i, ofs[0x40 + i * 4]);
		voice.operators[i].dt1_mul = ofs[0x40 + i * 8];
		voice.operators[i].tl      = ofs[0x60 + i * 8];
		voice.operators[i].ks_ar   = ofs[0x80 + i * 8];
		voice.operators[i].ame_d1r = ofs[0xa0 + i * 8];
		voice.operators[i].dt2_d2r = ofs[0xc0 + i * 8];
		voice.operators[i].d1l_rr  = ofs[0xe0 + i * 8];
	}

	uint8_t slot_masks[8] = { 0x08,0x08,0x08,0x08,0x0c,0x0e,0x0e,0x0f };
	int existing_voice = -1;
	for(int i = 0; i < num_opm_voices; i++) {
		struct opm_voice *v = &opm_voices[i];
		if(v->fb_connect != voice.fb_connect) continue;
		uint8_t slot_mask = slot_masks[voice.fb_connect & 0x07];
		int good = 1;
		for(int j = 0, m = 1; j < 4; j++, m <<= 1) {
			struct opm_voice_operator *o1 = &v->operators[j];
			struct opm_voice_operator *o2 = &voice.operators[j];
			if(
				o1->dt1_mul != o2->dt1_mul ||
				o1->ks_ar != o2->ks_ar ||
				o1->ame_d1r != o2->ame_d1r ||
				o1->dt2_d2r != o2->dt2_d2r ||
				o1->d1l_rr != o2->d1l_rr
			) {
				good = 0;
				break;
			}
			if(!(slot_mask & m)) {
				if(o1->tl != o2->tl) {
					good = 0;
					break;
				}
			}
		}
		if(!good) continue;

		existing_voice = i;
		break;
	}
	if(existing_voice < 0) {
		existing_voice = num_opm_voices++;
		opm_voices = realloc(opm_voices, num_opm_voices * sizeof(struct opm_voice));
		if(!opm_voices) {
			fprintf(stderr, "Could not reallocate %d OPN voices\n", num_opm_voices);
			return;
		}
		memcpy(&opm_voices[existing_voice], &voice, sizeof(voice));
	} else {
		struct opm_voice *v = &opm_voices[existing_voice];
		v->chan_used_mask |= 1 << (chan + port * 4);
		uint8_t slot_mask = slot_masks[v->fb_connect & 0x07];
		// find lowest attenuation, or highest volume
		for(int i = 0, m = 1; i < 4; i++, m <<= 1) {
			if(slot_mask & m) {
				if(voice.operators[i].tl < v->operators[i].tl)
					v->operators[i].tl = voice.operators[i].tl;
			}
		}
	}
}

enum {
	OPL = 1,
	OPN,
	OPM
};

enum {
	YM2413 = 1,
	YM2612,
	YM2151,
	YM2203,
	YM2608,
	YM2610,
	YM3812,
	YM3526,
	NUM_CHIPPIES
};

#define ALL_CHIPPOS \
	CHIPPY(YM2413, OPL, 0x51, 0xa1, 0) \
	CHIPPY(YM2612, OPN, 0x52, 0xa2, 0) \
	CHIPPY(YM2612, OPN, 0x53, 0xa3, 1) \
	CHIPPY(YM2151, OPM, 0x54, 0xa4, 0) \
	CHIPPY(YM2203, OPN, 0x55, 0xa5, 0) \
	CHIPPY(YM2608, OPN, 0x56, 0xa6, 0) \
	CHIPPY(YM2608, OPN, 0x57, 0xa7, 1) \
	CHIPPY(YM2610, OPN, 0x58, 0xa8, 0) \
	CHIPPY(YM2610, OPN, 0x59, 0xa9, 1) \
	CHIPPY(YM3812, OPN, 0x5a, 0xaa, 0) \
	CHIPPY(YM3526, OPN, 0x5b, 0xab, 0)

uint8_t ym_regs[NUM_CHIPPIES][2][512];
int ym_dac = 0;
void ym_write(int chip, int type, int which, int port, uint8_t reg, uint8_t data, int ofs) {
	// printf("ym_write chip=%d type=%d which=%d port=%d reg=%02x data=%02x\n",
		// chip, type, which, port, reg, data);
	ym_regs[chip][which][port * 256 + reg] = data;

	if(chip == YM2612 || chip == YM2610 || chip == YM2608 || chip == YM2203) {
		if(chip == YM2612 && port == 0 && reg == 0x2b) {
			ym_dac = data & 0x80 ? 1 : 0;
			return;
		}

		if(reg == 0x28 && port == 0) {
			uint8_t mask = data >> 4;
			uint8_t chan = data & 0x03;
			uint8_t key_port = data & 0x04 ? 1 : 0;

			if(mask) {
				// ignore DAC writes
				if(chip == YM2612 && chan == 2 && key_port == 1 && ym_dac == 1)
					return;
				push_opn_voice(ym_regs[chip][which], key_port, chan, mask, ofs);
			}
		}
	} else if(chip == YM2151) {
		if(reg == 0x08) {
			uint8_t mask = (data & 0x78) >> 3;
			uint8_t chan = data & 0x07;
			if(mask) {
				push_opm_voice(ym_regs[chip][which], 0, chan, mask, ofs);
			}
		}
	}
}

int main(int argc, char **argv) {
	for(int i = 1; i < argc; i++) {
		size_t data_len;
		uint8_t *data = load_gzfile(argv[i], &data_len);
		struct vgm_file vgm;
		vgm_file_load(&vgm, data, data_len);
		//printf("%s (%luB)\nversion=%x.%x\nEOF=%08x\nGD3=%08x\nVGM Data=%08x\nYM2413=%u\nYM2612=%u\nYM2151=%u\nYM2203=%u\nYM2608=%u\nYM2610=%u\nYM3812=%u\nYM3526=%u\n\n",
		//	argv[i], (unsigned long)data_len, vgm.version >> 8, vgm.version & 0xff, vgm.eof_offset, vgm.gd3_offset, vgm.data_offset,
		//	vgm.ym2413_clock, vgm.ym2612_clock, vgm.ym2151_clock, vgm.ym2203_clock,
		//	vgm.ym2608_clock, vgm.ym2610_clock, vgm.ym3812_clock, vgm.ym3526_clock);
		int remaining = data_len - vgm.data_offset;
		uint8_t *p = data + vgm.data_offset;
		int l;
		while((l = vgm_cmd_size(p, remaining)) >= 0) {
			if(l < 0) {
				fprintf(stderr, "Reached end of commands\n");
				continue;
			}

			switch(*p) {
#define CHIPPY(chip, type, byte0, byte1, port) \
				case byte0: ym_write(chip, type, 0, port, p[1], p[2], p - data); break; \
				case byte1: ym_write(chip, type, 1, port, p[1], p[2], p - data); break;
ALL_CHIPPOS
#undef CHIPPY
				default:
					break;
			}
			p += l;
			remaining -= l;
		}
		printf("//LFO: LFRQ AMD PMD WF NFRQ\n");
		printf("//@:[Num] [Name]\n");
		printf("//CH: PAN FL CON AMS PMS SLOT NE\n");
		printf("//OP: AR D1R D2R RR D1L TL KS MUL DT1 DT2 AMS-EN\n\n");

		int voice_num = 0;

		for(int j = 0; j < num_opn_voices; j++) {
			//opn_voice_dump(&opn_voices[j]);
			opn_voice_dump_opm(&opn_voices[j], voice_num);
			voice_num++;
		}
		free(opn_voices);
		opn_voices = 0;
		num_opn_voices = 0;

		for(int j = 0; j < num_opm_voices; j++) {
			opm_voice_dump_opm(&opm_voices[j], voice_num);
			voice_num++;
		}
		free(opm_voices);
		opm_voices = 0;
		num_opm_voices = 0;

		for(int j = voice_num; j < 128; j++) {
			printf(
				"@:%d no Name\n"
				"LFO: 0 0 0 0 0\n"
				"CH: 64 0 0 0 0 64 0\n"
				"M1: 31 0 0 4 0 0 0 1 0 0 0\n"
				"C1: 31 0 0 4 0 0 0 1 0 0 0\n"
				"M2: 31 0 0 4 0 0 0 1 0 0 0\n"
				"C2: 31 0 0 4 0 0 0 1 0 0 0\n"
				"\n",
				j
			);
		}
	}

	return 0;
}
