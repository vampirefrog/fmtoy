#include "syx_fb01.h"

#include <string.h>

#ifndef __EMSCRIPTEN__
#include <stdio.h>
#include <math.h>
#endif

const char *fb01_error_string(enum fb01_error err) {
	const char *errors[] = {
#define FB01_STATUS(s, d) d,
		FB01_ALL_STATUSES
#undef FB01_STATUS
	};
	if(err >= FB01_MAX_ERR) return "Unknown";
	return errors[err];
}

const char *fb01_input_controller_name(int c) {
	const char *names[] = {
		"OFF", "TOUCH", "WHEEL", "BREATH", "FOOT",
	};
	if(c < sizeof(names) / sizeof(names[0])) {
		return names[c];
	}
	return "Unknown";
}

const char *fb01_lfo_waveform_name(int w) {
	const char *names[] = {
		"sawtooth",
		"square",
		"triangle",
		"noise",
	};
	return names[w&0x03];
}

int fb01_midi_receive_voice(struct fb01_midi_receiver *rx, uint8_t byte) {
	switch(rx->voicestate) {
		case FB01_RX_V_IN_NAME:
			rx->namebuf[rx->voicepos++] = byte;
			if(rx->voicepos >= 0x08)
				rx->voicestate = FB01_RX_V_IN_PADDING;
			break;
		case FB01_RX_V_IN_PADDING:
			rx->voicepos++;
			if(rx->voicepos >= 0x20) {
				rx->voicestate = FB01_RX_V_IN_VOICE;
				rx->voicepos = 0;
			}
			break;
		case FB01_RX_V_IN_VOICE:
			if(rx->voicepos <= 6) rx->voice.name[rx->voicepos] = byte;
			else if(rx->voicepos >= 0x30) {
				switch(rx->voicepos) {
					case 0x3a: rx->voice.mono = byte >> 7; rx->voice.portamento_speed = byte & 0x7f; break;
					case 0x3b: rx->voice.pmd_controller = byte >> 4 & 0x07; rx->voice.pitch_bend_range = byte & 0x0f; break;
				}
			} else if(rx->voicepos >= 0x10) {
				int oppos = (rx->voicepos - 0x10) & 0x1f;
				struct fb01_bulk_voice_op *op = &rx->voice.op[oppos >> 3];
				switch(oppos & 0x07) {
					case 0x00: op->tl = byte & 0x7f; break;
					case 0x01: op->ks_type = byte >> 7; op->tl_sensitivity = byte >> 4 & 0x07; break;
					case 0x02: op->ks_level_depth = byte >> 4; op->tl_adjust = byte & 0x0f; break;
					case 0x03: op->ks_type |= byte >> 6 & 0x02; op->detune = byte >> 4 & 0x07; op->freq = byte & 0x0f; break;
					case 0x04: op->ks_rate_depth = byte >> 6; op->ar = byte & 0x1f; break;
					case 0x05: op->carrier = byte >> 7; op->ar_velocity_sens = byte >> 5 & 0x03; op->d1r = byte & 0x1f; break;
					case 0x06: op->inharmonic_freq = byte >> 6; op->d2r = byte & 0x1f; break;
					case 0x07: op->sl = byte >> 4; op->rr = byte & 0x0f; break;
				}
			} else switch(rx->voicepos) {
				case 0x07: rx->voice.user_code = byte; break;
				case 0x08: rx->voice.lfo_speed = byte; break;
				case 0x09: rx->voice.lfo_enable = byte >> 7; rx->voice.am_depth = byte & 0x7f; break;
				case 0x0a: rx->voice.lfo_sync = byte >> 7; rx->voice.pm_depth = byte & 0x7f; break;
				case 0x0b: rx->voice.op_mask = byte >> 3 & 0x0f; break;
				case 0x0c: rx->voice.l_enable = byte >> 7; rx->voice.r_enable = byte >> 6 & 0x01; rx->voice.fb_level = byte >> 3 & 0x07; rx->voice.algorithm = byte & 0x07; break;
				case 0x0d: rx->voice.pm_sens = byte >> 4 & 0x07; rx->voice.am_sens = byte & 0x03; break;
				case 0x0e: rx->voice.lfo_waveform = byte >> 5 & 0x03; break;
				case 0x0f: rx->voice.transpose = (int8_t)byte; break;
			}

			rx->voicepos++;
			if(rx->voicepos >= 64) {
				if(rx->voice_cb)
					rx->voice_cb(&rx->voice, rx->voicenum, rx->data_ptr);
				rx->voicepos = 0;
				rx->voicenum++;
				rx->voicestate = rx->voicenum >= 48 ? FB01_RX_V_AFTER_VOICES : FB01_RX_V_IN_VOICE;
			}
			break;
		case FB01_RX_V_AFTER_VOICES:
			return FB01_TOO_MANY_VOICES;
	}
	return FB01_IN_PROGRESS;
}

void fb01_midi_receiver_init(struct fb01_midi_receiver *rx) {
	rx->state = FB01_RX_SYSEX_START;
	rx->accumulator = 0;
	rx->voicestate = FB01_RX_V_IN_NAME;
}

int fb01_midi_receive(struct fb01_midi_receiver *rx, uint8_t byte) {
	switch(rx->state) {
		case FB01_RX_SYSEX_START:
			if(byte != 0xf0) return FB01_NOT_SYSEX;
			rx->state = FB01_RX_ID_NO;
			break;
		case FB01_RX_ID_NO:
			if(byte != 0x43) return FB01_BAD_ID_NO;
			rx->state = FB01_RX_SUB_STATUS;
			break;
		case FB01_RX_SUB_STATUS:
			if(byte < 16) {
				rx->state = FB01_RX_FORMAT_NO;
				rx->substatus = byte;
				rx->voicestate = FB01_RX_V_IN_NAME;
				rx->voicepos = rx->voicenum = 0;
			} else if(byte == 0x75) {
				rx->state = FB01_RX_SYSTEM_NO;
				rx->substatus = byte;
				rx->voicestate = FB01_RX_V_IN_NAME;
				rx->voicepos = rx->voicenum = 0;
			} else return FB01_BAD_SUBSTATUS;
			break;
		case FB01_RX_FORMAT_NO:
			if(byte != 0x0c) return FB01_BAD_FORMAT_NO;
			rx->state = FB01_RX_BYTE_COUNT_MSB_OR_EOX;
			break;
		case FB01_RX_SYSTEM_NO:
			if(byte >= 16) return FB01_BAD_SYSTEM_NO;
			rx->system_no = byte;
			rx->state = FB01_RX_MESSAGE_NO;
			break;
		case FB01_RX_MESSAGE_NO:
			if(byte != 0x00) return FB01_BAD_MESSAGE_NO;
			rx->state = FB01_RX_OP_NO;
			break;
		case FB01_RX_OP_NO:
			if(byte != 0x00) return FB01_BAD_OPERATION_NO;
			rx->state = FB01_RX_BANK_NO;
			break;
		case FB01_RX_BANK_NO:
			if(byte > 0x07) return FB01_BAD_BANK_NO;
			rx->bank_no = byte;
			rx->state = FB01_RX_BYTE_COUNT_MSB_OR_EOX;
			break;
		case FB01_RX_BYTE_COUNT_MSB_OR_EOX:
			if(byte == 0xf7) return FB01_SUCCESS;
			else if(byte & 0x80) return FB01_BAD_BYTE_COUNT_MSB;
			rx->byte_count = (byte & 0x7f) << 7;
			rx->state = FB01_RX_BYTE_COUNT_LSB;
			break;
		case FB01_RX_BYTE_COUNT_LSB:
			if(byte & 0x80) return FB01_BAD_BYTE_COUNT_LSB;
			rx->byte_count |= byte & 0x7f;
			rx->state = FB01_RX_DATA_LOW;
			rx->data_pos = 0;
			break;
		case FB01_RX_DATA_LOW:
			rx->accumulator += byte;
			if(byte > 0x0f) return FB01_BAD_DATA_LOW;
			rx->data_byte = byte;
			rx->data_pos++;
			rx->state = FB01_RX_DATA_HIGH;
			break;
		case FB01_RX_DATA_HIGH:
			rx->accumulator += byte;
			if(byte > 0x0f) return FB01_BAD_DATA_HIGH;
			rx->data_byte |= byte << 4;
			rx->data_pos++;
			rx->state = rx->data_pos >= rx->byte_count ? FB01_RX_CHECKSUM : FB01_RX_DATA_LOW;
			// only data type supported
			return fb01_midi_receive_voice(rx, rx->data_byte);
		case FB01_RX_CHECKSUM:
			if(byte != (-rx->accumulator & 0x7f)) return FB01_BAD_CHECKSUM;
			rx->state = FB01_RX_BYTE_COUNT_MSB_OR_EOX;
			rx->accumulator = 0;
			break;
		case FB01_RX_EOX:
			return FB01_AFTER_EOX;
	}
	return FB01_IN_PROGRESS;
}

static void voice_cb(struct fb01_bulk_voice *voice, int voicenum, void *data_ptr) {
	struct fb01_bulk_voice_bank *bank = (struct fb01_bulk_voice_bank *)data_ptr;
	if(voicenum < 48) memcpy(bank->voices + voicenum, voice, sizeof(*voice));
}

#ifndef __EMSCRIPTEN__
static uint8_t safechar(uint8_t c) {
	return (c > 48 && c < 127) || c == 0x20 ? c : '.';
}

float lfo_speed_to_hz(int lfo_speed, int clock) {
	return clock * (16 + (lfo_speed % 16)) / pow(2, 36 - floor(lfo_speed / 16.0));
}

void fb01_bulk_voice_dump(struct fb01_bulk_voice *voice, int voicenum) {
	printf(
		"Voice %d, %c%c%c%c%c%c%c algorithm: %d op_mask=%x\n",
		voicenum,
		safechar(voice->name[0]),
		safechar(voice->name[1]),
		safechar(voice->name[2]),
		safechar(voice->name[3]),
		safechar(voice->name[4]),
		safechar(voice->name[5]),
		safechar(voice->name[6]),
		voice->algorithm,
		voice->op_mask
	);
	printf("  LFO: %s speed: %d (%.02fHz) sync: %d waveform: %s (%d)\n", voice->lfo_enable ? "ENABLED" : "DISABLED", voice->lfo_speed, lfo_speed_to_hz(voice->lfo_speed, 4000000), voice->lfo_sync, fb01_lfo_waveform_name(voice->lfo_waveform), voice->lfo_waveform);
	printf("  am_depth: %d am_sens: %d pm_depth: %d pm_sens: %d\n", voice->am_depth, voice->am_sens, voice->pm_depth, voice->pm_sens);
	printf("  l_enable: %d r_enable: %d fb_level: %d\n", voice->l_enable, voice->r_enable, voice->fb_level);
	printf("  mono: %d transpose: %d pmd_controller: %s (%d)\n", voice->mono, voice->transpose, fb01_input_controller_name(voice->pmd_controller), voice->pmd_controller);
	printf("  pitch_bend_range: %d portamento_speed: %d\n", voice->pitch_bend_range, voice->portamento_speed);
	printf("     Attack -Decay-         --TL--- Key sns           \n");
	printf("  OP AR  VS D1R D2R  RR SL  TL S  A T LD RD DT  F C IF\n");
	for(int j = 0; j < 4; j++) {
		printf(
			"   %d% 3d% 4d% 4d% 4d% 4d% 3d% 4d% 2d% 3d% 2d% 3d% 3d% 3d% 3d% 2d% 3d\n",
			j,
			voice->op[j].ar,
			voice->op[j].ar_velocity_sens,
			voice->op[j].d1r,
			voice->op[j].d2r,
			voice->op[j].rr,
			voice->op[j].sl,
			voice->op[j].tl,
			voice->op[j].tl_sensitivity,
			voice->op[j].tl_adjust,
			voice->op[j].ks_type,
			voice->op[j].ks_level_depth,
			voice->op[j].ks_rate_depth,
			voice->op[j].detune,
			voice->op[j].freq,
			voice->op[j].carrier,
			voice->op[j].inharmonic_freq
		);
	}
}

void fb01_bulk_voice_bank_dump(struct fb01_bulk_voice_bank *bank) {
	printf(
		"bank %d, channel %d, name=%c%c%c%c%c%c%c%c voices=%d\n",
		bank->bank,
		bank->channel,
		safechar(bank->name[0]),
		safechar(bank->name[1]),
		safechar(bank->name[2]),
		safechar(bank->name[3]),
		safechar(bank->name[4]),
		safechar(bank->name[5]),
		safechar(bank->name[6]),
		safechar(bank->name[7]),
		bank->num_voices
	);
	for(int i = 0; i < bank->num_voices; i++) {
		struct fb01_bulk_voice *voice = bank->voices + i;
		fb01_bulk_voice_dump(voice, i);
	}
}
#endif

int fb01_bulk_voice_bank_from_buffer(struct fb01_bulk_voice_bank *bank, uint8_t *buf, size_t filesize) {
	struct fb01_midi_receiver rx;
	fb01_midi_receiver_init(&rx);
	rx.data_ptr = (void *)bank;
	rx.voice_cb = voice_cb;
	for(int i = 0; i < filesize; i++) {
		int r = fb01_midi_receive(&rx, buf[i]);
		if(r == FB01_SUCCESS) {
			if(i < filesize - 1)
				return FB01_EARLY_EOX;
		} else if(r != FB01_IN_PROGRESS) {
			return r;
		}
	}
	memcpy(bank->name, rx.namebuf, 8);
	bank->bank = rx.bank_no;
	bank->channel = rx.system_no;
	bank->num_voices = rx.voicenum;

	return FB01_SUCCESS;
}

#define PUTSIZE(s) { buf[bufpos++] = s >> 7 & 0x7f; buf[bufpos++] = s & 0x7f; }
#define PUTBYTE(b) { int _b = b; accumulator += (buf[bufpos++] = _b & 0x0f); accumulator += (buf[bufpos++] = _b >> 4); }
#define PUTCHECKSUM { buf[bufpos++] = -accumulator & 0x7f; accumulator = 0; }
#define FLUSHBUF { int r = write(buf, bufpos, data_ptr); if(r < bufpos) return FB01_SHORT_WRITE; bufpos = 0; }
int fb01_bulk_voice_bank_send(struct fb01_bulk_voice_bank *bank, size_t (*write)(void *, size_t, void *), void *data_ptr) {
	uint8_t buf[131];
	buf[0] = 0xf0;
	buf[1] = 0x43;
	buf[2] = 0x75;
	buf[3] = bank->channel & 0x0f;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = bank->bank & 0x07;
	size_t r;
	r = write(buf, 7, data_ptr);
	if(r < 7) return FB01_SHORT_WRITE;

	int bufpos = 0;
	uint8_t accumulator = 0;

	PUTSIZE(64);
	for(int i = 0; i < 32; i++) {
		PUTBYTE(i < 8 ? bank->name[i] : 0);
	}
	PUTCHECKSUM;
	FLUSHBUF;

	accumulator = bufpos = 0;
	for(int i = 0; i < bank->num_voices; i++) {
		PUTSIZE(128);
		struct fb01_bulk_voice *voice = bank->voices + i;
		for(int j = 0; j < 7; j++)
			PUTBYTE(voice->name[j]);
		PUTBYTE(voice->user_code);
		PUTBYTE(voice->lfo_speed);
		PUTBYTE(voice->lfo_enable << 7 | voice->am_depth);
		PUTBYTE(voice->lfo_sync << 7 | voice->pm_depth);
		PUTBYTE(voice->op_mask << 3);
		PUTBYTE(voice->l_enable << 7 | voice->r_enable << 6 | voice->fb_level << 3 | voice->algorithm);
		PUTBYTE(voice->pm_sens << 4 | voice->am_sens);
		PUTBYTE(voice->lfo_waveform << 5);
		PUTBYTE((uint8_t)voice->transpose);
		for(int j = 0; j < 4; j++) {
			struct fb01_bulk_voice_op *op = voice->op + j;
			PUTBYTE(op->tl & 0x7f);
			PUTBYTE((op->ks_type & 0x01) << 7 | op->tl_sensitivity << 4);
			PUTBYTE(op->ks_level_depth << 4 | op->tl_adjust);
			PUTBYTE((op->ks_type & 0x02) << 6 | op->detune << 4 | op->freq);
			PUTBYTE(op->ks_rate_depth << 6 | op->ar);
			PUTBYTE(op->carrier << 7 | op->ar_velocity_sens << 5 | op->d1r);
			PUTBYTE(op->inharmonic_freq << 6 | op->d2r);
			PUTBYTE(op->sl << 4 | op->rr);
		}
		for(int j = 0x70; j <= 0x79; j++)
			PUTBYTE(0);
		PUTBYTE(voice->mono << 7 | voice->portamento_speed);
		PUTBYTE(voice->pmd_controller << 4 | voice->pitch_bend_range);
		for(int j = 0x7C; j <= 0x7F; j++)
			PUTBYTE(0);
		PUTCHECKSUM;
		FLUSHBUF;
	}
	buf[bufpos++] = 0xf7;
	FLUSHBUF;

	return FB01_SUCCESS;
}
