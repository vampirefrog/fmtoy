#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fmtoy.h"
#include "tools.h"
#include "opm_file.h"

static void fmtoy_set_buf_size(struct fmtoy *fmtoy, int size) {
	if(fmtoy->buf_size < size) {
		fmtoy->buf_size = size;
		fmtoy->render_buf_l = realloc(fmtoy->render_buf_l, size);
		fmtoy->render_buf_r = realloc(fmtoy->render_buf_r, size);
		fmtoy->chip_buf_l = realloc(fmtoy->chip_buf_l, size);
		fmtoy->chip_buf_r = realloc(fmtoy->chip_buf_r, size);
	}
}

static void ssg_set_clock(void *param, int clock) { (void)param; (void) clock; }
static void ssg_write(void *param, int address, int data) { (void)param; (void)address; (void)data; }
static void ssg_read(void *param) { (void)param; }
static void ssg_reset(void *param) { (void)param; }

void fmtoy_init(struct fmtoy *fmtoy, int sample_rate) {
	fmtoy->sample_rate = sample_rate;

	fmtoy->render_buf_l = fmtoy->render_buf_r = 0;
	fmtoy->chip_buf_l = fmtoy->chip_buf_r = 0;
	fmtoy_set_buf_size(fmtoy, 1024);

	ssg_callbacks cb = {
		set_clock: ssg_set_clock,
		write: ssg_write,
		read: ssg_read,
		reset: ssg_reset
	};

	fmtoy->ym2151 = ym2151_init(4000000, sample_rate);
	ym2151_reset_chip(fmtoy->ym2151);
	fmtoy->ym2203 = ym2203_init(0, 3579545, sample_rate, 0, 0, &cb);
	ym2203_reset_chip(fmtoy->ym2203);
	fmtoy->ym2608 = ym2608_init(0, 3579545, sample_rate, 0, 0, &cb);
	ym2608_reset_chip(fmtoy->ym2608);
	fmtoy->ym2610 = ym2610_init(0, 3579545, sample_rate, 0, 0, &cb);
	ym2610_reset_chip(fmtoy->ym2610);
	fmtoy->ym2612 = ym2612_init(0, 3579545, sample_rate, 0, 0);
	ym2612_reset_chip(fmtoy->ym2612);
}

void fmtoy_load_voice(struct fmtoy *fmtoy, char *filename) {
	int l = strlen(filename);
	if(!strncasecmp(filename + l - 4, ".opm", 4)) {
		printf("loading voice %s\n", filename);
		struct opm_file opm;
		size_t l;
		uint8_t *data = load_file(filename, &l);
		opm_file_load(&opm, data, l);
		for(int i = 0; i < OPM_FILE_MAX_VOICES; i++) {
			if(!opm.voices[i].used) continue;
			if(fmtoy->num_voices > 127) continue;
			struct opm_file_voice *v = &opm.voices[i];

			int unused = 1;
			for(int k = 0; k < 4; k++) {
				if(
					v->operators[k].ar != 31 ||
					v->operators[k].d1r != 0 ||
					v->operators[k].d2r != 0 ||
					v->operators[k].rr != 4 ||
					v->operators[k].d1l != 0 ||
					v->operators[k].tl != 0 ||
					v->operators[k].ks != 0 ||
					v->operators[k].mul != 1 ||
					v->operators[k].dt1 != 0 ||
					v->operators[k].dt2 != 0 ||
					v->operators[k].ams_en != 0
				) {
					unused = 0;
					break;
				}
			}

			if(unused &&
				v->lfo_lfrq == 0 &&
				v->lfo_amd == 0 &&
				v->lfo_pmd == 0 &&
				v->lfo_wf == 0 &&
				v->lfo_nfrq == 0 &&
				v->ch_pan == 64 &&
				v->ch_fl == 0 &&
				v->ch_con == 0 &&
				v->ch_ams == 0 &&
				v->ch_pms == 0 &&
				v->ch_slot == 64 &&
				v->ch_ne == 0
			) {
				continue;
			}

			// OPM voices
			struct fmtoy_opm_voice *opmv = &fmtoy->opm_voices[fmtoy->num_voices];
			opmv->rl_fb_con = 3 << 6 | v->ch_fl << 3 | v->ch_con;
			opmv->pms_ams = v->ch_pms << 4 | v->ch_ams;
			int ops[4] = { 0, 2, 1, 3 };
			for(int j = 0; j < 4; j++) {
				struct fmtoy_opm_voice_operator *fop = &opmv->operators[j];
				struct opm_file_operator *op = &v->operators[ops[j]];
				fop->dt1_mul = op->dt1 << 4 | op->mul;
				fop->tl = op->tl;
				fop->ks_ar = op->ks << 6 | op->ar;
				fop->ams_en_d1r = op->ams_en << 7 | op->d1r;
				fop->dt2_d2r = op->dt2 << 6 | op->d2r;
				fop->d1l_rr = op->d1l << 4 | op->rr;
			}

			printf("loaded opm voice %d\n", fmtoy->num_voices);
			fmtoy->num_voices++;

			// OPN voices
			//struct fmtoy_opn_voice *opnv = &fmtoy->opn_voices[fmtoy->num_voices];
			// opnv->lr_fl_con = 3 << 6 | v->ch_fl << 3 | v->ch_con;
			// opnv->pms_ams = v->ch_pms << 4 | v->ch_ams;
			// for(int j = 0; j < 4; j++) {
			// 	struct fmtoy_opm_voice_operator *fop = fv->operators[j];
			// 	struct opm_file_voice_operator *op = v->operators[j];
			// 	fop->dt1_mul = op->dt1 << 4 | op->mul;
			// 	fop->tl = op->tl;
			// 	fop->ks_ar = op->ks << 6 | op->ar;
			// 	fop->ams_en_d1r = op->ams_en << 7 | op->d1r;
			// 	fop->dt2_d2r = op->dt2 << 6 | op->d2r;
			// 	fop->d1l_rr = op->d1l << 4 | op->rr;
			// }
		}
	}
}

void fmtoy_program_change(struct fmtoy *fmtoy, uint8_t channel, uint8_t program) {
	switch(channel) {
		case 0: {
			struct fmtoy_opm_voice *v = &fmtoy->opm_voices[program];
			for(int i = 0; i < 8; i++) {
				ym2151_write_reg(fmtoy->ym2151, 0x20 + i, v->rl_fb_con);
				ym2151_write_reg(fmtoy->ym2151, 0x38 + i, v->pms_ams);
				for(int j = 0; j < 4; j++) {
					struct fmtoy_opm_voice_operator *op = &v->operators[j];
					ym2151_write_reg(fmtoy->ym2151, 0x40 + i + j * 8, op->dt1_mul);
					ym2151_write_reg(fmtoy->ym2151, 0x60 + i + j * 8, op->tl);
					ym2151_write_reg(fmtoy->ym2151, 0x80 + i + j * 8, op->ks_ar);
					ym2151_write_reg(fmtoy->ym2151, 0xa0 + i + j * 8, op->ams_en_d1r);
					ym2151_write_reg(fmtoy->ym2151, 0xc0 + i + j * 8, op->dt2_d2r);
					ym2151_write_reg(fmtoy->ym2151, 0xe0 + i + j * 8, op->d1l_rr);
				}
			}
		}
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
	}
}

void fmtoy_pitch_bend(struct fmtoy *fmtoy, uint8_t channel, int bend) {
}

void fmtoy_cc(struct fmtoy *fmtoy, uint8_t channel, int cc, int value) {
}

static int find_unused_channel(struct fmtoy_chip_channel *channels, int num_channels) {
	int chip_channel = 0;
	uint32_t min_frames = UINT32_MAX;
	for(int i = 0; i < 8; i++) {
		if(!channels[i].on) {
			chip_channel = i;
			break;
		}
		if(channels[i].frames < min_frames) {
			min_frames = channels[i].frames;
			chip_channel = i;
		}
	}

	return chip_channel;
}

static int find_used_channel(struct fmtoy_chip_channel *channels, int num_channels, int note) {
	int chip_channel = -1;

	for(int i = 0; i < num_channels; i++)
		if(channels[i].note == note)
			return i;

	return chip_channel;
}

// just a bogus function,
// returns unique ever increasing values,
// like a timer would
static uint32_t frame_time() {
	static uint32_t t = 1;
	return t++;
}

void fmtoy_note_on(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity) {
	switch(channel) {
		case 0: {
			int chip_channel = find_unused_channel(fmtoy->ym2151_channels, sizeof(fmtoy->ym2151_channels) / sizeof(fmtoy->ym2151_channels[0]));
			uint8_t opm_notes[12] = { 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14 };
			ym2151_write_reg(fmtoy->ym2151, 0x28 + chip_channel, (note / 12 - 2) << 4 | opm_notes[note % 12]);
			ym2151_write_reg(fmtoy->ym2151, 0x30 + chip_channel, 0x05);
			ym2151_write_reg(fmtoy->ym2151, 0x08, 0x78 + chip_channel);
			fmtoy->ym2151_channels[chip_channel].frames = frame_time();
			fmtoy->ym2151_channels[chip_channel].on = 1;
			fmtoy->ym2151_channels[chip_channel].note = note;
		}
			break;
	}

}

void fmtoy_note_off(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity) {
	int chip_channel;
	switch(channel) {
		case 0:
			chip_channel = find_used_channel(fmtoy->ym2151_channels, sizeof(fmtoy->ym2151_channels) / sizeof(fmtoy->ym2151_channels[0]), note);
			if(chip_channel >= 0) {
				ym2151_write_reg(fmtoy->ym2151, 0x08, chip_channel & 0x07);
				fmtoy->ym2151_channels[chip_channel].on = 0;
			}
			break;
		case 1:
			chip_channel = find_used_channel(fmtoy->ym2203_channels, sizeof(fmtoy->ym2203_channels) / sizeof(fmtoy->ym2203_channels[0]), note);
			if(chip_channel >= 0) {
				ym2203_write(fmtoy->ym2203, 0x28, chip_channel < 3);
				fmtoy->ym2203_channels[chip_channel].on = 0;
			}
			break;
		case 2:
			chip_channel = find_used_channel(fmtoy->ym2608_channels, sizeof(fmtoy->ym2608_channels) / sizeof(fmtoy->ym2608_channels[0]), note);
			if(chip_channel >= 0) {
				ym2608_write(fmtoy->ym2608, 0x28, chip_channel < 3 ? chip_channel : chip_channel + 1);
				fmtoy->ym2608_channels[chip_channel].on = 0;
			}
			break;
		case 3:
			chip_channel = find_used_channel(fmtoy->ym2610_channels, sizeof(fmtoy->ym2610_channels) / sizeof(fmtoy->ym2610_channels[0]), note);
			if(chip_channel >= 0) {
				ym2610_write(fmtoy->ym2610, 0x28, chip_channel);
				fmtoy->ym2610_channels[chip_channel].on = 0;
			}
			break;
		case 4:
			chip_channel = find_used_channel(fmtoy->ym2612_channels, sizeof(fmtoy->ym2612_channels) / sizeof(fmtoy->ym2612_channels[0]), note);
			if(chip_channel >= 0) {
				ym2612_write(fmtoy->ym2612, 0x28, chip_channel < 3 ? chip_channel : chip_channel + 1);
				fmtoy->ym2612_channels[chip_channel].on = 0;
			}
			break;
	}
}

void fmtoy_render(struct fmtoy *fmtoy, int samples) {
	fmtoy_set_buf_size(fmtoy, samples);

	memset(fmtoy->render_buf_l, 0, sizeof(fmtoy->render_buf_l[0]) * samples);
	memset(fmtoy->render_buf_r, 0, sizeof(fmtoy->render_buf_r[0]) * samples);
	stream_sample_t *chipBufs[2] = { fmtoy->chip_buf_l, fmtoy->chip_buf_r };

#define ACCUMULATE(x, y) { x += (y); if(x > 32767) x = 32767; else if(x < -32768) x = -32768; }
#define MIX_CHIP \
	for(int i = 0; i < samples; i++) { \
		ACCUMULATE(fmtoy->render_buf_l[i], fmtoy->chip_buf_l[i]); \
		ACCUMULATE(fmtoy->render_buf_r[i], fmtoy->chip_buf_r[i]); \
	}

	ym2151_update_one(fmtoy->ym2151, chipBufs, samples);
	MIX_CHIP
	ym2203_update_one(fmtoy->ym2203, chipBufs, samples);
	MIX_CHIP
	ym2608_update_one(fmtoy->ym2608, chipBufs, samples);
	MIX_CHIP
	ym2610_update_one(fmtoy->ym2610, chipBufs, samples);
	MIX_CHIP
	ym2612_update_one(fmtoy->ym2612, chipBufs, samples);
	MIX_CHIP
}
