#include "syx_dx21.h"

#include <string.h>

#ifndef __EMSCRIPTEN__
#include <stdio.h>
#include <math.h>
#endif

const char *dx21_error_string(enum dx21_error err) {
	const char *errors[] = {
#define DX21_STATUS(s, d) d,
		DX21_ALL_STATUSES
#undef DX21_STATUS
	};
	if(err >= DX21_MAX_ERR) return "Unknown";
	return errors[err];
}

const char *dx21_input_controller_name(int c) {
	const char *names[] = {
		"OFF", "TOUCH", "WHEEL", "BREATH", "FOOT",
	};
	if(c < sizeof(names) / sizeof(names[0])) {
		return names[c];
	}
	return "Unknown";
}

const char *dx21_lfo_waveform_name(int w) {
	const char *names[] = {
		"sawtooth",
		"square",
		"triangle",
		"sample & hold",
	};
	return names[w&0x03];
}

int dx21_midi_receive_vmem_voice(struct dx21_midi_receiver *rx, uint8_t byte) {
	if(rx->voicenum >= 32) return DX21_TOO_MANY_VOICES;
	if(rx->voicepos < 40) {
		struct dx21_vced_voice_op *op = &rx->voice.op[rx->voicepos / 10];
		int oppos = rx->voicepos % 10;
		switch(oppos) {
			case 0: if(byte > 31) return DX21_BAD_VMEM_AR; op->ar = byte; break;
			case 1: if(byte > 31) return DX21_BAD_VMEM_D1R; op->d1r = byte; break;
			case 2: if(byte > 31) return DX21_BAD_VMEM_D2R; op->d2r = byte; break;
			case 3: if(byte > 15) return DX21_BAD_VMEM_RR; op->rr = byte; break;
			case 4: if(byte > 15) return DX21_BAD_VMEM_D1L; op->d1l= byte; break;
			case 5: if(byte > 99) return DX21_BAD_VMEM_KSL; op->ksl = byte; break;
			case 6: if(byte & 0x80) return DX21_BAD_VMEM_AME_EBS_KVS; op->ame = byte >> 6 & 0x01; op->eg_bias_sens = byte >> 3 & 0x07; op->kv = byte & 0x07; break;
			case 7: if(byte > 99) return DX21_BAD_VMEM_OL; op->ol = byte; break;
			case 8: if(byte > 63) return DX21_BAD_VMEM_FREQ; op->freq = byte; break;
			case 9: if(byte > 32) return DX21_BAD_VMEM_RS_DET; if((byte & 0x07) > 6) return DX21_BAD_VMEM_DET; op->ksr = byte >> 3 & 0x03; op->detune = byte & 0x07; break;
		}
	} else if(rx->voicepos >= 57 && rx->voicepos <= 66) {
		rx->voice.name[rx->voicepos-57] = byte;
	} else if(rx->voicepos >= 73 && rx->voicepos <= 80) {
		struct dx21_vced_voice_op *op = &rx->voice.op[(rx->voicepos - 73) / 2];
		int oppos = (rx->voicepos - 73) % 2;
		switch(oppos) {
			case 0: if((byte & 0xc0) != 0) return DX21_BAD_SHFT_FIX_FIXRG; op->shft = byte >> 4 & 0x03; op->fix = byte >> 3 & 0x01; op->fix_range = byte & 0x07; break;
			case 1: if((byte & 0x80) != 0) return DX21_BAD_OPW_FINE; op->opw = byte >> 4 & 0x07; op->fin_ratio = byte & 0x0f; break;
		}
	} else switch(rx->voicepos) {
		case 40: rx->voice.lfo_sync = byte >> 6; rx->voice.feedback = byte >> 3 & 0x07; rx->voice.algorithm = byte & 0x07; break;
		case 41: if(byte > 99) return DX21_BAD_LFO_SPEED; rx->voice.lfo_speed = byte; break;
		case 42: if(byte > 99) return DX21_BAD_LFO_DELAY; rx->voice.lfo_delay = byte; break;
		case 43: if(byte > 99) return DX21_BAD_LFO_PMD; rx->voice.lfo_pmd = byte; break;
		case 44: if(byte > 99) return DX21_BAD_LFO_AMD; rx->voice.lfo_amd = byte; break;
		case 45: rx->voice.pm_sens = byte >> 4; rx->voice.am_sens = byte >> 2 & 0x03; rx->voice.lfo_wave = byte & 0x03; break;
		case 46: if(byte > 48) return DX21_BAD_MIDDLE_C; rx->voice.middle_c = byte; break;
		case 47: if(byte > 12) return DX21_BAD_PITCH_BEND_RANGE; rx->voice.pitch_bend_range = byte; break;
		case 48: if(byte > 0x1f) return DX21_BAD_CH_MO_SU_PO_PM; rx->voice.chorus = byte >> 4 & 0x01; rx->voice.mono_mode = byte >> 3 & 0x01; rx->voice.fc_sustain = byte >> 2 & 0x01; rx->voice.fc_porta = byte >> 1 & 0x01; rx->voice.fingered_porta = byte & 0x01; break;
		case 49: if(byte > 99) return DX21_BAD_PORTA_TIME; rx->voice.porta_time = byte; break;
		case 50: if(byte > 99) return DX21_BAD_FC_VOLUME; rx->voice.fc_volume = byte; break;
		case 51: if(byte > 99) return DX21_BAD_MW_PITCH; rx->voice.mw_pitch = byte; break;
		case 52: if(byte > 99) return DX21_BAD_MW_AMPLITUDE; rx->voice.mw_amplitude = byte; break;
		case 53: if(byte > 99) return DX21_BAD_BC_PITCH; rx->voice.bc_pitch = byte; break;
		case 54: if(byte > 99) return DX21_BAD_BC_AMPLITUDE; rx->voice.bc_amplitude = byte; break;
		case 55: if(byte > 99) return DX21_BAD_BC_PITCH_BIAS; rx->voice.bc_pitch_bias = byte; break;
		case 56: if(byte > 99) return DX21_BAD_BC_EG_BIAS; rx->voice.bc_eg_bias = byte; break;
		case 67: if(byte > 99) return DX21_BAD_PEG_RATE_1; rx->voice.peg_rate_1 = byte; break;
		case 68: if(byte > 99) return DX21_BAD_PEG_RATE_2; rx->voice.peg_rate_2 = byte; break;
		case 69: if(byte > 99) return DX21_BAD_PEG_RATE_3; rx->voice.peg_rate_3 = byte; break;
		case 70: if(byte > 99) return DX21_BAD_LEVEL_1; rx->voice.level_1 = byte; break;
		case 71: if(byte > 99) return DX21_BAD_LEVEL_2; rx->voice.level_2 = byte; break;
		case 72: if(byte > 99) return DX21_BAD_LEVEL_3; rx->voice.level_3 = byte; break;
		case 81: if(byte > 7) return DX21_BAD_REVERB_RATE; rx->voice.reverb_rate = byte; break;
		case 82: if(byte > 99) return DX21_BAD_FC_PITCH; rx->voice.fc_pitch = byte; break;
		case 83: if(byte > 99) return DX21_BAD_FC_AMPLITUDE; rx->voice.fc_amplitude = byte; break;
	}

	rx->voicepos++;
	if(rx->voicepos >= 128) {
		if(rx->voice_cb)
			rx->voice_cb(&rx->voice, rx->voicenum, rx->data_ptr);
		rx->voicepos = 0;
		rx->voicenum++;
	}
	return DX21_IN_PROGRESS;
}

int dx21_midi_receive_vced_voice(struct dx21_midi_receiver *rx, uint8_t byte) {
	printf("RECV VOICE 0x%02x voicenum=%d voicepos=%d\n", byte, rx->voicenum, rx->voicepos);
	if(rx->voicenum >= 32) return DX21_TOO_MANY_VOICES;
	if(rx->voicepos < 52) {
		struct dx21_vced_voice_op *op = &rx->voice.op[rx->voicepos / 13];
		int oppos = rx->voicepos % 13;
		switch(oppos) {
			case 0: if(byte > 31) return DX21_BAD_VCED_AR; op->ar = byte; break;
			case 1: if(byte > 31) return DX21_BAD_VCED_D1R; op->d1r = byte; break;
			case 2: if(byte > 31) return DX21_BAD_VCED_D2R; op->d2r = byte; break;
			case 3: if(byte > 15) return DX21_BAD_VCED_RR; op->rr = byte; break;
			case 4: if(byte > 15) return DX21_BAD_VCED_D1L; op->d1l= byte; break;
			case 5: if(byte > 99) return DX21_BAD_VCED_KSL; op->ksl = byte; break;
			case 6: if(byte > 3) return DX21_BAD_VCED_KSR; op->ksr = byte; break;
			case 7: if(byte > 7) return DX21_BAD_VCED_EG_BIAS_SENS; op->eg_bias_sens = byte; break;
			case 8: if(byte > 1) return DX21_BAD_VCED_AME; op->ame = byte; break;
			case 9: if(byte > 7) return DX21_BAD_VCED_KV; op->kv = byte; break;
			case 10: if(byte > 99) return DX21_BAD_VCED_OL; op->ol = byte; break;
			case 11: if(byte > 63) return DX21_BAD_VCED_FREQ; op->freq = byte; break;
			case 12: if(byte > 6) return DX21_BAD_VCED_DETUNE; op->detune = byte; break;
		}
	} else if(rx->voicepos >= 77 && rx->voicepos <= 86) {
		rx->voice.name[rx->voicepos-77] = byte;
	} else switch(rx->voicepos) {
		case 52: if(byte > 7) return DX21_BAD_VCED_ALGORITHM; rx->voice.algorithm = byte; break;
		case 53: if(byte > 7) return DX21_BAD_FEEDBACK; rx->voice.feedback = byte; break;
		case 54: if(byte > 99) return DX21_BAD_LFO_SPEED; rx->voice.lfo_speed = byte; break;
		case 55: if(byte > 99) return DX21_BAD_LFO_DELAY; rx->voice.lfo_delay = byte; break;
		case 56: if(byte > 99) return DX21_BAD_LFO_PMD; rx->voice.lfo_pmd = byte; break;
		case 57: if(byte > 99) return DX21_BAD_LFO_AMD; rx->voice.lfo_amd = byte; break;
		case 58: if(byte > 1) return DX21_BAD_LFO_SYNC; rx->voice.lfo_sync = byte; break;
		case 59: if(byte > 3) return DX21_BAD_LFO_WAVE; rx->voice.lfo_wave = byte; break;
		case 60: if(byte > 7) return DX21_BAD_Pm_SEnS; rx->voice.pm_sens = byte; break;
		case 61: if(byte > 3) return DX21_BAD_Am_SEnS; rx->voice.am_sens = byte; break;
		case 62: if(byte > 48) return DX21_BAD_MIDDLE_C; rx->voice.middle_c = byte; break;
		case 63: if(byte > 1) return DX21_BAD_MONO_MODE; rx->voice.mono_mode = byte; break;
		case 64: if(byte > 12) return DX21_BAD_PITCH_BEND_RANGE; rx->voice.pitch_bend_range = byte; break;
		case 65: if(byte > 1) return DX21_BAD_FINGeRED_pORTA; rx->voice.fingered_porta = byte; break;
		case 66: if(byte > 99) return DX21_BAD_PORTA_TIME; rx->voice.porta_time = byte; break;
		case 67: if(byte > 99) return DX21_BAD_FC_VOLUME; rx->voice.fc_volume = byte; break;
		case 68: if(byte > 1) return DX21_BAD_FC_SUSTAIN; rx->voice.fc_sustain = byte; break;
		case 69: if(byte > 1) return DX21_BAD_FC_PORTA; rx->voice.fc_porta = byte; break;
		case 70: if(byte > 1) return DX21_BAD_CHORUS; rx->voice.chorus = byte; break;
		case 71: if(byte > 99) return DX21_BAD_MW_PITCH; rx->voice.mw_pitch = byte; break;
		case 72: if(byte > 99) return DX21_BAD_MW_AMPLITUDE; rx->voice.mw_amplitude = byte; break;
		case 73: if(byte > 99) return DX21_BAD_BC_PITCH; rx->voice.bc_pitch = byte; break;
		case 74: if(byte > 99) return DX21_BAD_BC_AMPLITUDE; rx->voice.bc_amplitude = byte; break;
		case 75: if(byte > 99) return DX21_BAD_BC_PITCH_BIAS; rx->voice.bc_pitch_bias = byte; break;
		case 76: if(byte > 99) return DX21_BAD_BC_EG_BIAS; rx->voice.bc_eg_bias = byte; break;
		case 87: if(byte > 99) return DX21_BAD_PEG_RATE_1; rx->voice.peg_rate_1 = byte; break;
		case 88: if(byte > 99) return DX21_BAD_PEG_RATE_2; rx->voice.peg_rate_2 = byte; break;
		case 89: if(byte > 99) return DX21_BAD_PEG_RATE_3; rx->voice.peg_rate_3 = byte; break;
		case 90: if(byte > 99) return DX21_BAD_LEVEL_1; rx->voice.level_1 = byte; break;
		case 91: if(byte > 99) return DX21_BAD_LEVEL_2; rx->voice.level_2 = byte; break;
		case 92: if(byte > 99) return DX21_BAD_LEVEL_3; rx->voice.level_3 = byte; break;
	}

	rx->voicepos++;
	if(rx->voicepos >= 128) {
		if(rx->voice_cb)
			rx->voice_cb(&rx->voice, rx->voicenum, rx->data_ptr);
		rx->voicepos = 0;
		rx->voicenum++;
	}
	return DX21_IN_PROGRESS;
}

void dx21_midi_receiver_init(struct dx21_midi_receiver *rx) {
	rx->state = DX21_RX_SYSEX_START;
	rx->accumulator = 0;

	rx->voicepos = rx->voicenum = 0;
}

int dx21_midi_receive(struct dx21_midi_receiver *rx, uint8_t byte) {
	switch(rx->state) {
		case DX21_RX_SYSEX_START:
			if(byte != 0xf0) return DX21_NOT_SYSEX;
			rx->state = DX21_RX_ID_NO;
			break;
		case DX21_RX_ID_NO:
			if(byte != 0x43) return DX21_BAD_ID_NO;
			rx->state = DX21_RX_SUB_STATUS;
			break;
		case DX21_RX_SUB_STATUS:
			if(byte > 15) return DX21_BAD_SUBSTATUS;
			rx->state = DX21_RX_FORMAT_NO;
			rx->channel = byte;
			break;
		case DX21_RX_FORMAT_NO:
			if(byte != 0x03 && byte != 0x04)
				return DX21_BAD_FORMAT_NO;
			rx->format = byte;
			rx->state = DX21_RX_BYTE_COUNT_MSB_OR_EOX;
			break;
		case DX21_RX_BYTE_COUNT_MSB_OR_EOX:
			if(byte == 0xf7) return DX21_SUCCESS;
			else if(byte & 0x80) return DX21_BAD_BYTE_COUNT_MSB;
			rx->byte_count = (byte & 0x7f) << 7;
			rx->state = DX21_RX_BYTE_COUNT_LSB;
			break;
		case DX21_RX_BYTE_COUNT_LSB:
			if(byte & 0x80) return DX21_BAD_BYTE_COUNT_LSB;
			rx->byte_count |= byte & 0x7f;
			rx->state = DX21_RX_DATA;
			rx->data_pos = 0;
			break;
		case DX21_RX_DATA:
			rx->accumulator += byte;
			rx->data_pos++;
			if(rx->data_pos >= rx->byte_count)
				rx->state = DX21_RX_CHECKSUM;

			if(rx->format == 0x03) // one voice
				return dx21_midi_receive_vmem_voice(rx, byte);
			else /* 0x04 */ // 32 voices
				return dx21_midi_receive_vmem_voice(rx, byte);
		case DX21_RX_CHECKSUM:
			if(byte != (-rx->accumulator & 0x7f)) return DX21_BAD_CHECKSUM;
			rx->state = DX21_RX_BYTE_COUNT_MSB_OR_EOX;
			rx->accumulator = 0;
			break;
		case DX21_RX_EOX:
			return DX21_AFTER_EOX;
	}
	return DX21_IN_PROGRESS;
}

static void voice_cb(struct dx21_vced_voice *voice, int voicenum, void *data_ptr) {
	struct dx21_vced_voice_bank *bank = (struct dx21_vced_voice_bank *)data_ptr;
	if(voicenum < 48) memcpy(bank->voices + voicenum, voice, sizeof(*voice));
}

#ifndef __EMSCRIPTEN__
static uint8_t safechar(uint8_t c) {
	return (c > 48 && c < 127) || c == 0x20 ? c : '.';
}

float lfo_speed_to_hz(int lfo_speed, int clock) {
	return clock * (16 + (lfo_speed % 16)) / pow(2, 36 - floor(lfo_speed / 16.0));
}

void dx21_vced_voice_dump(struct dx21_vced_voice *voice, int voicenum) {
	printf(
		"Voice %d: %c%c%c%c%c%c%c%c%c%c\n",
		voicenum,
		safechar(voice->name[0]), safechar(voice->name[1]),
		safechar(voice->name[2]), safechar(voice->name[3]),
		safechar(voice->name[4]), safechar(voice->name[5]),
		safechar(voice->name[6]), safechar(voice->name[7]),
		safechar(voice->name[8]), safechar(voice->name[9])
	);

	printf("  Algorithm: %d\n", voice->algorithm+1);
	printf("  LFO speed=%d delay=%d pmd=%d amd=%d sync=%d wave=%s (%d)\n", voice->lfo_speed, voice->lfo_delay, voice->lfo_pmd, voice->lfo_amd, voice->lfo_sync, dx21_lfo_waveform_name(voice->lfo_wave), voice->lfo_wave);
	printf("  PM sensitivity: %d  AM sensitivity: %d\n", voice->pm_sens, voice->am_sens);
	const char *notes = "CCDDEFFGGAAB";
	printf("  Chorus: %d  Reverb: %d\n", voice->chorus, voice->reverb_rate);
	printf("  Portamento: %s, time=%d\n", voice->fingered_porta ? "Fingered" : "Full time", voice->porta_time);
	printf("  Pitch bend range: %d  Middle C: %c%d (%d)  Play mode: %s\n", voice->pitch_bend_range, notes[voice->middle_c % 12], voice->middle_c / 12 + 1, (int)voice->middle_c - 24, voice->mono_mode ? "MONO" : "POLY");
	printf("  FC volume=%d sustain=%d portamento=%d pitch=%d amplitude=%d\n", voice->fc_volume, voice->fc_sustain, voice->fc_porta, voice->fc_pitch, voice->fc_amplitude);
	printf("  Mod wheel pitch=%d amplitude=%d\n", voice->mw_pitch, voice->mw_amplitude);
	printf("  Breath pitch=%d amplitude=%d pitch bias=%d eg bias=%d\n", voice->bc_pitch, voice->bc_amplitude, voice->bc_pitch_bias - 50, voice->bc_eg_bias);
	printf("  Pitch EG rate %d, %d, %d level %d, %d, %d\n", voice->peg_rate_1, voice->peg_rate_2, voice->peg_rate_2, voice->level_1, voice->level_2, voice->level_3);

	printf("  OP AR D1R D2R RR D1L LS RS EBS AMS KV OL  F DT FIX FIXR FIN OPW SHFT\n");
	int opmap[] = { 3, 1, 2, 0 };
	for(int j = 0; j < 4; j++) {
		struct dx21_vced_voice_op *op = voice->op + opmap[j];
		printf(
			"  "     // indentation
			"%1d "   // Operator number
			" %2d"   // EG AR        0-31
			"  %2d"  // EG D1R       0-31
			"  %2d"  // EG D2R       0-31
			" %2d"   // EG RR        0-15
			"  %2d"  // EG D1L       0-15
			" %2d"   // LEVEL SCALE  0-99
			"  %1d"  // RATE SCALE   0-3
			"   %1d" // E BIAS SENS. 0-1
			"   %1d" // A MOD SENS.  0-1
			"  %1d"  // KEY VELOCITY 0-7
			" %2d"   // OUTPUT LEVEL 0-99
			" %2d"   // FREQUENCY    0-63
			" %+2d"  // DETUNE       0-7
			"   %d"  // FIX          0-1
			"    %d" // Fix Range    0-7
			"  %2d"  // FIN (RATIO)  0-15
			"   %d"  // OPW          0-7
			"    %d" // SHFT         0-3
			"\n",
			j+1,
			op->ar,
			op->d1r,
			op->d2r,
			op->rr,
			op->d1l,
			op->ksl,
			op->ksr,
			op->eg_bias_sens,
			op->ame,
			op->kv,
			op->ol,
			op->freq,
			op->detune - 3,
			op->fix,
			op->fix_range,
			op->fin_ratio,
			op->opw,
			op->shft
		);
	}
}

void dx21_vced_voice_bank_dump(struct dx21_vced_voice_bank *bank) {
	printf(
		"channel %d, voices=%d\n",
		bank->channel,
		bank->num_voices
	);
	for(int i = 0; i < bank->num_voices; i++) {
		struct dx21_vced_voice *voice = bank->voices + i;
		dx21_vced_voice_dump(voice, i);
	}
}
#endif

int dx21_vced_voice_bank_from_buffer(struct dx21_vced_voice_bank *bank, uint8_t *buf, size_t filesize) {
	struct dx21_midi_receiver rx;
	dx21_midi_receiver_init(&rx);
	rx.data_ptr = (void *)bank;
	rx.voice_cb = voice_cb;
	for(int i = 0; i < filesize; i++) {
		int r = dx21_midi_receive(&rx, buf[i]);
		if(r == DX21_SUCCESS) {
			if(i < filesize - 1)
				return DX21_EARLY_EOX;
		} else if(r != DX21_IN_PROGRESS) {
			return r;
		}
	}
	bank->channel = rx.channel;
	bank->num_voices = rx.voicenum;

	return DX21_SUCCESS;
}

#define PUTBYTE(b) buf[bufpos++] = b
#define PUTSIZE(s) PUTBYTE(s >> 7 & 0x7f); PUTBYTE(s & 0x7f);
#define PUTDATABYTE(b) { uint8_t _b = b; accumulator += _b; PUTBYTE(_b); }
#define PUTCHECKSUM { buf[bufpos++] = -accumulator & 0x7f; accumulator = 0; }
#define FLUSHBUF { size_t r = write(buf, bufpos, data_ptr); if(r < bufpos) return DX21_SHORT_WRITE; bufpos = 0; }
int dx21_vced_voice_bank_send(struct dx21_vced_voice_bank *bank, size_t (*write)(void *, size_t, void *), void *data_ptr) {
	uint8_t accumulator = 0;
	uint8_t buf[4 + 2 + 4096 + 1 + 1];
	int bufpos = 0;

	PUTBYTE(0xf0);
	PUTBYTE(0x43);
	PUTBYTE(bank->channel & 0x0f);
	PUTBYTE(0x04);

	PUTSIZE(4096);
	for(int i = 0; i < bank->num_voices; i++) {
		struct dx21_vced_voice *voice = bank->voices + i;

		for(int j = 0; j < 4; j++) {
			struct dx21_vced_voice_op *op = voice->op + j;
			PUTDATABYTE(op->ar);
			PUTDATABYTE(op->d1r);
			PUTDATABYTE(op->d2r);
			PUTDATABYTE(op->rr);
			PUTDATABYTE(op->d1l);
			PUTDATABYTE(op->ksl);
			PUTDATABYTE(op->ame << 6 | op->eg_bias_sens << 3 | op->kv);
			PUTDATABYTE(op->ol);
			PUTDATABYTE(op->freq);
			PUTDATABYTE(op->ksr << 3 | op->detune);
		}

		PUTDATABYTE(voice->lfo_sync << 6 | voice->feedback << 3 | voice->algorithm);
		PUTDATABYTE(voice->lfo_speed);
		PUTDATABYTE(voice->lfo_delay);
		PUTDATABYTE(voice->lfo_pmd);
		PUTDATABYTE(voice->lfo_amd);
		PUTDATABYTE(voice->pm_sens << 4 | voice->am_sens << 2 | voice->lfo_wave);
		PUTDATABYTE(voice->middle_c);
		PUTDATABYTE(voice->pitch_bend_range);
		PUTDATABYTE(voice->chorus << 4 | voice->mono_mode << 3 | voice->fc_sustain << 2 | voice->fc_porta << 1 | voice->fingered_porta);
		PUTDATABYTE(voice->porta_time);
		PUTDATABYTE(voice->fc_volume);
		PUTDATABYTE(voice->mw_pitch);
		PUTDATABYTE(voice->mw_amplitude);
		PUTDATABYTE(voice->bc_pitch);
		PUTDATABYTE(voice->bc_amplitude);
		PUTDATABYTE(voice->bc_pitch_bias);
		PUTDATABYTE(voice->bc_eg_bias);
		for(int j = 0; j < 10; j++)
			PUTDATABYTE(voice->name[j]);
		PUTDATABYTE(voice->peg_rate_1);
		PUTDATABYTE(voice->peg_rate_2);
		PUTDATABYTE(voice->peg_rate_3);
		PUTDATABYTE(voice->level_1);
		PUTDATABYTE(voice->level_2);
		PUTDATABYTE(voice->level_3);

		for(int j = 0; j < 4; j++) {
			struct dx21_vced_voice_op *op = voice->op + j;
			PUTDATABYTE(op->shft << 4 | op->fix << 3 | op->fix_range);
			PUTDATABYTE(op->opw << 4 | op->fin_ratio);
		}

		PUTDATABYTE(voice->reverb_rate);
		PUTDATABYTE(voice->fc_pitch);
		PUTDATABYTE(voice->fc_amplitude);

		for(int j = 84; j < 128; j++)
			PUTDATABYTE(0);
	}
	PUTCHECKSUM;

	PUTBYTE(0xf7);
	FLUSHBUF;

	return DX21_SUCCESS;
}
