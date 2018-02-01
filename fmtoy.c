#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "fmtoy.h"
#include "tools.h"
#include "opm_file.h"

#include "fmtoy_ym2151.h"
#include "fmtoy_ym2203.h"
#include "fmtoy_ym2608.h"
#include "fmtoy_ym2610.h"
#include "fmtoy_ym2610b.h"
#include "fmtoy_ym2612.h"

static void fmtoy_set_buf_size(struct fmtoy *fmtoy, int size) {
	if(fmtoy->buf_size < size) {
		fmtoy->buf_size = size;
		fmtoy->render_buf_l = realloc(fmtoy->render_buf_l, size * sizeof(*fmtoy->render_buf_l));
		fmtoy->render_buf_r = realloc(fmtoy->render_buf_r, size * sizeof(*fmtoy->render_buf_r));
		fmtoy->chip_buf_l = realloc(fmtoy->chip_buf_l, size * sizeof(*fmtoy->chip_buf_l));
		fmtoy->chip_buf_r = realloc(fmtoy->chip_buf_r, size * sizeof(*fmtoy->chip_buf_r));
	}
}

void fmtoy_init(struct fmtoy *fmtoy, int sample_rate) {
	fmtoy->sample_rate = sample_rate;

	fmtoy->render_buf_l = fmtoy->render_buf_r = 0;
	fmtoy->chip_buf_l = fmtoy->chip_buf_r = 0;
	fmtoy_set_buf_size(fmtoy, 1024);

	fmtoy->channels[0].chip = &fmtoy_chip_ym2151;
	fmtoy->channels[1].chip = &fmtoy_chip_ym2203;
	fmtoy->channels[2].chip = &fmtoy_chip_ym2608;
	fmtoy->channels[3].chip = &fmtoy_chip_ym2610;
	fmtoy->channels[4].chip = &fmtoy_chip_ym2610b;
	fmtoy->channels[5].chip = &fmtoy_chip_ym2612;

	for(int i = 0; i < 16; i++) {
		if(fmtoy->channels[i].chip && fmtoy->channels[i].chip->init) {
			fmtoy->channels[i].chip->init(fmtoy, fmtoy->sample_rate, &fmtoy->channels[i]);
		}
	}
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


			// OPN voices
			struct fmtoy_opn_voice *opnv = &fmtoy->opn_voices[fmtoy->num_voices];

			opnv->lfo = v->lfo_lfrq >> 4;
			opnv->fb_connect = v->ch_fl << 3 | v->ch_con;
			opnv->lr_ams_pms = 3 << 6 | v->ch_ams << 4 | v->ch_pms;
			for(int j = 0; j < 4; j++) {
				struct fmtoy_opn_voice_operator *fop = &opnv->operators[j];
				struct opm_file_operator *op = &v->operators[ops[j]];
				fop->dt_multi = op->dt1 << 4 | op->mul;
				fop->tl = op->tl;
				fop->ks_ar = op->ks << 6 | op->ar;
				fop->am_dr = op->ams_en << 7 | op->d1r;
				fop->sr = op->d2r;
				fop->sl_rr = op->d1l << 4 | op->rr;
				fop->ssg_eg = 0;
			}

			fmtoy->num_voices++;
		}
	}
}

void fmtoy_program_change(struct fmtoy *fmtoy, uint8_t channel, uint8_t program) {
	// change on all channels
	for(int i = 0; i < 16; i++) {
		if(fmtoy->channels[i].chip)
			fmtoy->channels[i].chip->program_change(fmtoy, program, &fmtoy->channels[i]);
	}

//	if(channel < 16 && fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->program_change) {
//	}
}

void fmtoy_pitch_bend(struct fmtoy *fmtoy, uint8_t channel, int bend) {
	if(channel < 16 && fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->pitch_bend) {
		fmtoy->channels[channel].chip->pitch_bend(fmtoy, bend, &fmtoy->channels[channel]);
	}
}

void fmtoy_cc(struct fmtoy *fmtoy, uint8_t channel, int cc, int value) {
}

static int find_unused_channel(struct fmtoy_chip_channel *channels, int num_channels) {
	int chip_channel = 0;
	uint32_t min_frames = UINT32_MAX;
	for(int i = 0; i < num_channels; i++) {
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

static float midi_note_freq(uint8_t note) {
	return (440.0 / 32.0) * (pow(2, ((note - 9) / 12.0)));
}

void fmtoy_note_on(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity) {
	if(channel < 16 && fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->note_on) {
		int chip_channel = find_unused_channel(fmtoy->channels[channel].chip->channels, fmtoy->channels[channel].chip->max_poliphony);
		fmtoy->channels[channel].chip->note_on(fmtoy, chip_channel, note, velocity, &fmtoy->channels[channel]);
		fmtoy->channels[channel].chip->channels[chip_channel].frames = frame_time();
		fmtoy->channels[channel].chip->channels[chip_channel].on = 1;
		fmtoy->channels[channel].chip->channels[chip_channel].note = note;
	}
}

void fmtoy_note_off(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity) {
	if(channel < 16 && fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->note_on) {
		int chip_channel = find_used_channel(fmtoy->channels[channel].chip->channels, fmtoy->channels[channel].chip->max_poliphony, note);
		if(chip_channel >= 0) {
			fmtoy->channels[channel].chip->note_off(fmtoy, chip_channel, note, velocity, &fmtoy->channels[channel]);
			fmtoy->channels[channel].chip->channels[chip_channel].on = 0;
		}
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

	for(int i = 0; i < 16; i++) {
		if(fmtoy->channels[i].chip && fmtoy->channels[i].chip->render) {
			fmtoy->channels[i].chip->render(fmtoy, chipBufs, samples, &fmtoy->channels[i]);
			MIX_CHIP
		}
	}
}

const char *fmtoy_channel_name(struct fmtoy *fmtoy, uint8_t channel) {
	if(fmtoy->channels[channel].chip)
		return fmtoy->channels[channel].chip->name;

	return 0;
}
