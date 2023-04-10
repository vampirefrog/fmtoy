#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdio.h>

#include "fmtoy.h"
#include "midi.h"

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

struct fmtoy *fmtoy_new(int clock, int sample_rate) {
	struct fmtoy *t = malloc(sizeof(struct fmtoy));
	if(!t) return 0;

	fmtoy_init(t, clock, sample_rate);

	return t;
}

void fmtoy_init(struct fmtoy *fmtoy, int clock, int sample_rate) {
	fmtoy->num_voices = 0;
	memset(fmtoy->opm_voices, 0, sizeof(fmtoy->opm_voices));
	memset(fmtoy->opn_voices, 0, sizeof(fmtoy->opn_voices));
	fmtoy->sample_rate = sample_rate;
	fmtoy->clock = clock;
	fmtoy->pitch_bend_range = 2;

	fmtoy->render_buf_l = fmtoy->render_buf_r = 0;
	fmtoy->chip_buf_l = fmtoy->chip_buf_r = 0;

	fmtoy->channels[0].chip = &fmtoy_chip_ym2151;
	fmtoy->channels[1].chip = &fmtoy_chip_ym2203;
	fmtoy->channels[2].chip = &fmtoy_chip_ym2608;
	fmtoy->channels[3].chip = &fmtoy_chip_ym2610;
	fmtoy->channels[4].chip = &fmtoy_chip_ym2610b;
	fmtoy->channels[5].chip = &fmtoy_chip_ym2612;

	for(int i = 0; i < 16; i++) {
		if(fmtoy->channels[i].chip && fmtoy->channels[i].chip->init) {
			fmtoy->channels[i].chip->init(fmtoy, fmtoy->clock, fmtoy->sample_rate, &fmtoy->channels[i]);
		}
	}
}

void fmtoy_opm_voice_to_fmtoy_opn_voice(struct fmtoy_opm_voice *opmv, struct fmtoy_opn_voice *opnv) {
	/* per chip registers */	
	opnv->lfo = opmv->lfrq >> 4;

	/* per channel registers */
	opnv->fb_connect = (opmv->rl_fb_con & 0x38) | (opmv->rl_fb_con & 0x07);
	opnv->lr_ams_pms = (opmv->rl_fb_con & 0xc0) | (opmv->pms_ams & 0x03) << 4 | (opmv->pms_ams & 0x70) >> 3;

	/* slot mask */
	opnv->sm = opmv->sm << 1;

	/* operators */
	for(int j = 0; j < 4; j++) {
		struct fmtoy_opn_voice_operator *nop = &opnv->operators[j];
		struct fmtoy_opm_voice_operator *mop = &opmv->operators[j];

		nop->dt_mul = mop->dt1_mul & 0x7f;
		nop->tl = mop->tl & 0x7f;
		nop->ks_ar = mop->ks_ar;
		nop->am_dr = mop->ame_d1r;
		nop->sr = mop->dt2_d2r & 0x1f;
		nop->sl_rr = mop->d1l_rr;
		nop->ssg_eg = 0;
	}
}

void fmtoy_load_opm_voice(struct fmtoy *fmtoy, int voice_num, struct fmtoy_opm_voice *voice) {
	/* Load OPM voice */
	struct fmtoy_opm_voice *opmv = &fmtoy->opm_voices[fmtoy->num_voices];
	memcpy(opmv, voice, sizeof(*voice));

	/* And convert to OPN voice as well */
	struct fmtoy_opn_voice *opnv = &fmtoy->opn_voices[fmtoy->num_voices];
	fmtoy_opm_voice_to_fmtoy_opn_voice(opmv, opnv);
}

void fmtoy_append_opm_voice(struct fmtoy *fmtoy, struct fmtoy_opm_voice *voice) {
	if(fmtoy->num_voices > 127) return;
	fmtoy_load_opm_voice(fmtoy, fmtoy->num_voices, voice);
	fmtoy->num_voices++;
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

static float float_note_freq(float note) {
	return (440.0 / 32.0) * (pow(2.0, ((note - 9.0) / 12.0)));
}

void fmtoy_note_on(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity) {
	if(channel < 16 && fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->note_on) {
		int chip_channel = find_unused_channel(fmtoy->channels[channel].chip->channels, fmtoy->channels[channel].chip->max_poliphony);
		float pitch = float_note_freq((float)note + (float)fmtoy->channels[channel].pitch_bend * (float)fmtoy->pitch_bend_range / 8191.0);
		fmtoy->channels[channel].chip->note_on(fmtoy, chip_channel, pitch, velocity, &fmtoy->channels[channel]);
		fmtoy->channels[channel].chip->channels[chip_channel].frames = frame_time();
		fmtoy->channels[channel].chip->channels[chip_channel].on = 1;
		fmtoy->channels[channel].chip->channels[chip_channel].note = note;
	}
}

void fmtoy_note_off(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity) {
	if(channel < 16 && fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->note_on) {
		int chip_channel = find_used_channel(fmtoy->channels[channel].chip->channels, fmtoy->channels[channel].chip->max_poliphony, note);
		if(chip_channel >= 0) {
			fmtoy->channels[channel].chip->note_off(fmtoy, chip_channel, velocity, &fmtoy->channels[channel]);
			fmtoy->channels[channel].chip->channels[chip_channel].on = 0;
		}
	}
}

void fmtoy_pitch_bend(struct fmtoy *fmtoy, uint8_t channel, int bend) {
	fmtoy->channels[channel].pitch_bend = bend;
	if(channel < 16 && fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->note_on) {
		for(int i = 0; i < fmtoy->channels[channel].chip->max_poliphony; i++) {
			if(!fmtoy->channels[channel].chip->channels[i].on) continue;
			float pitch = float_note_freq((float)fmtoy->channels[channel].chip->channels[i].note + (float)fmtoy->channels[channel].pitch_bend * (float)fmtoy->pitch_bend_range / 8191.0);
			fmtoy->channels[channel].chip->pitch_bend(fmtoy, i, pitch, &fmtoy->channels[channel]);
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

stream_sample_t *fmtoy_get_buf_l(struct fmtoy *fmtoy) {
	return fmtoy->render_buf_l;
}

stream_sample_t *fmtoy_get_buf_r(struct fmtoy *fmtoy) {
	return fmtoy->render_buf_r;
}

const char *fmtoy_channel_name(struct fmtoy *fmtoy, uint8_t channel) {
	if(fmtoy->channels[channel].chip)
		return fmtoy->channels[channel].chip->name;

	return 0;
}

struct fmtoy_opm_voice *fmtoy_opm_voice_new() {
	struct fmtoy_opm_voice *v = malloc(sizeof(*v));
	if(!v) return 0;

	fmtoy_opm_voice_init(v);

	return v;
}

int fmtoy_opm_voice_free(struct fmtoy_opm_voice *voice) {
	if(!voice) return -1;

	free(voice);
	return 0;
}

void fmtoy_opm_voice_init(struct fmtoy_opm_voice *v) {
	memset(v, 0, sizeof(*v));
}

void fmtoy_opm_voice_set_lfo(struct fmtoy_opm_voice *voice, uint8_t lfrq, uint8_t pmd, uint8_t amd, uint8_t w) {
	voice->lfrq = lfrq;
	voice->pmd = pmd & 0x7f;
	voice->amd = amd & 0x7f;
	voice->w = w & 0x03;
}

void fmtoy_opm_voice_set_noise(struct fmtoy_opm_voice *voice, uint8_t ne, uint8_t nfrq) {
	voice->ne_nfrq = (ne > 0 ? 0x80 : 0x00) | (nfrq & 0x1f);
}

void fmtoy_opm_voice_set_pan(struct fmtoy_opm_voice *voice, uint8_t rl) {
	voice->rl_fb_con = (voice->rl_fb_con & 0x3f) | ((rl & 0x03) << 6);
}

void fmtoy_opm_voice_set_feedback(struct fmtoy_opm_voice *voice, uint8_t fb) {
	voice->rl_fb_con = (voice->rl_fb_con & 0xc7) | ((fb & 0x07) << 3);
}

void fmtoy_opm_voice_set_connection(struct fmtoy_opm_voice *voice, uint8_t con) {
	voice->rl_fb_con = (voice->rl_fb_con & 0xf8) | (con & 0x07);
}

void fmtoy_opm_voice_set_pms_ams(struct fmtoy_opm_voice *voice, uint8_t pms, uint8_t ams) {
	voice->pms_ams = (pms & 0x07) << 4 | (ams & 0x03);
}

void fmtoy_opm_voice_set_slot_mask(struct fmtoy_opm_voice *voice, uint8_t sm) {
	voice->sm = (sm & 0x0f) << 3;
}

void fmtoy_opm_voice_set_operator_dt1_mul(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t dt1, uint8_t mul) {
	if(op >= 4) return;
	voice->operators[op].dt1_mul = (dt1 & 0x07) << 4 | (mul & 0x0f);
}

void fmtoy_opm_voice_set_operator_tl(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t tl) {
	if(op >= 4) return;
	voice->operators[op].tl = tl;
}

void fmtoy_opm_voice_set_operator_ks_ar(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t ks, uint8_t ar) {
	if(op >= 4) return;
	voice->operators[op].ks_ar = (ks & 0x03) << 6 | (ar & 0x1f);
}

void fmtoy_opm_voice_set_operator_ame_d1r(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t ame, uint8_t d1r) {
	if(op >= 4) return;
	voice->operators[op].ame_d1r = (ame & 0x01) << 7 | (d1r & 0x1f);
}

void fmtoy_opm_voice_set_operator_dt2_d2r(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t dt2, uint8_t d2r) {
	if(op >= 4) return;
	voice->operators[op].dt2_d2r = (dt2 & 0x03) << 6 | (d2r & 0x1f);
}

void fmtoy_opm_voice_set_operator_d1l_rr(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t d1l, uint8_t rr) {
	if(op >= 4) return;
	voice->operators[op].d1l_rr = (d1l & 0x0f) << 4 | (rr & 0x0f);
}

#ifndef __EMSCRIPTEN__
void fmtoy_opm_voice_dump(struct fmtoy_opm_voice *voice) {
	printf("voice %p \"%s\"\n", voice, voice->name);

	printf("lfrq=0x%02x, pmd=0x%02x, amd=0x%02x, w=0x%02x, ne_nfrq=0x%02x\n",
		voice->lfrq, voice->pmd, voice->amd, voice->w, voice->ne_nfrq);

	printf("rl_fb_con=0x%02x, pms_ams=0x%02x\n",
		voice->rl_fb_con, voice->pms_ams);

	printf("sm=0x%02x\n", voice->sm);

	for(int i = 0; i < 4; i++) {
		printf("op %d dt1_mul=0x%02x, tl=0x%02x, ks_ar=0x%02x, ame_d1r=0x%02x, dt2_d2r=0x%02x, d1l_rr=0x%02x\n",
			i, voice->operators[i].dt1_mul, voice->operators[i].tl, voice->operators[i].ks_ar, voice->operators[i].ame_d1r, voice->operators[i].dt2_d2r, voice->operators[i].d1l_rr);
	}
}
#endif
