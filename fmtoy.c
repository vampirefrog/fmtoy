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
#include "fmtoy_ym3812.h"

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
	fmtoy->channels[5].chip = &fmtoy_chip_ym2612;
	fmtoy->channels[6].chip = &fmtoy_chip_ym3812;

	fmtoy->lfo_clock_period = sample_rate / 100; // every 10 ms fire the timer
	fmtoy->lfo_clock_phase = 0;

	for(int i = 0; i < 16; i++) {
		if(fmtoy->channels[i].chip && fmtoy->channels[i].chip->init) {
			fmtoy->channels[i].chip->init(fmtoy, fmtoy->clock, fmtoy->sample_rate, &fmtoy->channels[i]);
		}
	}
}

void fmtoy_destroy(struct fmtoy *fmtoy) {
	for(int i = 0; i < 16; i++) {
		if(fmtoy->channels[i].chip && fmtoy->channels[i].chip->destroy) {
			fmtoy->channels[i].chip->destroy(fmtoy, &fmtoy->channels[i]);
		}
	}
}

void fmtoy_load_opm_voice(struct fmtoy *fmtoy, int voice_num, struct fmtoy_opm_voice *voice) {
	/* Load OPM voice */
	struct fmtoy_opm_voice *opmv = &fmtoy->opm_voices[voice_num];
	memcpy(opmv, voice, sizeof(*voice));

	/* And convert to OPN voice as well */
	struct fmtoy_opn_voice *opnv = &fmtoy->opn_voices[voice_num];
	fmtoy_opm_voice_to_fmtoy_opn_voice(opmv, opnv);
}

void fmtoy_append_opm_voice(struct fmtoy *fmtoy, struct fmtoy_opm_voice *voice) {
	if(fmtoy->num_voices > 127) return;
	fmtoy_load_opm_voice(fmtoy, fmtoy->num_voices, voice);
	fmtoy->num_voices++;
}

void fmtoy_load_opl_voice(struct fmtoy *fmtoy, int voice_num, struct fmtoy_opl_voice *voice) {
	/* Load OPL voice */
	struct fmtoy_opl_voice *oplv = &fmtoy->opl_voices[voice_num];
	memcpy(oplv, voice, sizeof(*voice));

	/* And convert to OPN voice as well */
	// struct fmtoy_opn_voice *opnv = &fmtoy->opn_voices[voice_num];
	// fmtoy_opl_voice_to_fmtoy_opn_voice(oplv, opnv);
}

void fmtoy_append_opl_voice(struct fmtoy *fmtoy, struct fmtoy_opl_voice *voice) {
	if(fmtoy->num_voices > 127) return;
	fmtoy_load_opl_voice(fmtoy, fmtoy->num_voices, voice);
	fmtoy->num_voices++;
}

void fmtoy_program_change(struct fmtoy *fmtoy, uint8_t channel, uint8_t program) {
	// change on all channels
	fmtoy->channels[channel].program = program;
	if(fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->program_change)
		fmtoy->channels[channel].chip->program_change(fmtoy, program, &fmtoy->channels[channel]);
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
	if(velocity == 0) {
		fmtoy_note_off(fmtoy, channel, note, velocity);
		return;
	}

	if(channel >= 16) return;
	if(!fmtoy->channels[channel].chip) return;
	if(!fmtoy->channels[channel].chip->note_on) return;

	int chip_channel = find_unused_channel(fmtoy->channels[channel].chip->channels, fmtoy->channels[channel].chip->max_poliphony);
	fmtoy->channels[channel].chip->channels[chip_channel].note = note;
	fmtoy->channels[channel].chip->channels[chip_channel].frames = frame_time();
	fmtoy->channels[channel].chip->channels[chip_channel].on = 1;
	float pitch = float_note_freq((float)fmtoy->channels[channel].chip->channels[chip_channel].note + (float)fmtoy->channels[channel].pitch_bend * (float)fmtoy->pitch_bend_range / 8191.0);
	fmtoy->channels[channel].chip->channels[chip_channel].pitch = pitch;
	fmtoy->channels[channel].chip->note_on(fmtoy, chip_channel, pitch, velocity, &fmtoy->channels[channel]);
}

void fmtoy_note_off(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity) {
	if(channel >= 16) return;
	if(!fmtoy->channels[channel].chip) return;
	if(!fmtoy->channels[channel].chip->note_off) return;
	int chip_channel = find_used_channel(fmtoy->channels[channel].chip->channels, fmtoy->channels[channel].chip->max_poliphony, note);
	if(chip_channel < 0) return;
	fmtoy->channels[channel].chip->channels[chip_channel].on = 0;
	fmtoy->channels[channel].chip->note_off(fmtoy, chip_channel, velocity, &fmtoy->channels[channel]);
}

void fmtoy_pitch_bend(struct fmtoy *fmtoy, uint8_t channel, int bend) {
	fmtoy->channels[channel].pitch_bend = bend;
	if(channel < 16 && fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->pitch_bend) {
		for(int i = 0; i < fmtoy->channels[channel].chip->max_poliphony; i++) {
			if(!fmtoy->channels[channel].chip->channels[i].on) continue;
			float pitch = float_note_freq((float)fmtoy->channels[channel].chip->channels[i].note + (float)fmtoy->channels[channel].pitch_bend * (float)fmtoy->pitch_bend_range / 8191.0);
			fmtoy->channels[channel].chip->channels[i].pitch = pitch;
			fmtoy->channels[channel].chip->pitch_bend(fmtoy, i, pitch, &fmtoy->channels[channel]);
		}
	}
}

void fmtoy_mod_wheel(struct fmtoy *fmtoy, uint8_t channel, int mod) {
	if(channel < 16 && fmtoy->channels[channel].chip && fmtoy->channels[channel].chip->mod_wheel)
		fmtoy->channels[channel].chip->mod_wheel(fmtoy, mod, &fmtoy->channels[channel]);
}

void fmtoy_cc(struct fmtoy *fmtoy, uint8_t channel, int cc, int value) {
	switch(cc) {
		case 0x01:
			fmtoy_mod_wheel(fmtoy, channel, value);
			break;
	}
}

static void fmtoy_set_buf_size(struct fmtoy *fmtoy, int size) {
	if(fmtoy->buf_size >= size) return;
	fmtoy->buf_size = size;
	fmtoy->render_buf_l = realloc(fmtoy->render_buf_l, size * sizeof(*fmtoy->render_buf_l));
	fmtoy->render_buf_r = realloc(fmtoy->render_buf_r, size * sizeof(*fmtoy->render_buf_r));
	fmtoy->chip_buf_l = realloc(fmtoy->chip_buf_l, size * sizeof(*fmtoy->chip_buf_l));
	fmtoy->chip_buf_r = realloc(fmtoy->chip_buf_r, size * sizeof(*fmtoy->chip_buf_r));
}

// handle portamento and software LFO
static void fmtoy_timer_tick(struct fmtoy *fmtoy) {
	for(int i = 0; i < 16; i++) {
		struct fmtoy_chip *chip = fmtoy->channels[i].chip;
		if(!chip) continue;
		for(int j = 0; j < chip->max_poliphony; j++) {
			struct fmtoy_chip_channel *channel = chip->channels + j;
			if(channel->on) {

			}
		}
	}
}

void fmtoy_render(struct fmtoy *fmtoy, int samples) {
	fmtoy_set_buf_size(fmtoy, samples);
	memset(fmtoy->render_buf_l, 0, sizeof(fmtoy->render_buf_l[0]) * samples);
	memset(fmtoy->render_buf_r, 0, sizeof(fmtoy->render_buf_r[0]) * samples);
	stream_sample_t *chipBufs[2] = { fmtoy->chip_buf_l, fmtoy->chip_buf_r };
	stream_sample_t *renderBufs[2] = { fmtoy->render_buf_l, fmtoy->render_buf_r };
	for(int s = fmtoy->lfo_clock_phase - fmtoy->lfo_clock_period; s < samples; s += fmtoy->lfo_clock_period) {
		int render_samples = fmtoy->lfo_clock_period;
		if(s < 0) render_samples += s;
		else if(s + render_samples > samples) {
			render_samples = samples - s;
			fmtoy->lfo_clock_phase = fmtoy->lfo_clock_period - render_samples;
			fmtoy_timer_tick(fmtoy);
		}

		for(int i = 0; i < 16; i++) {
			if(!fmtoy->channels[i].chip) continue;
			if(!fmtoy->channels[i].chip->render) continue;
			fmtoy->channels[i].chip->render(fmtoy, chipBufs, render_samples, &fmtoy->channels[i]);
			for(int j = 0; j < render_samples; j++) {
				renderBufs[0][j] += chipBufs[0][j];
				renderBufs[1][j] += chipBufs[1][j];
			}
		}

		chipBufs[0] += render_samples;
		chipBufs[1] += render_samples;
		renderBufs[0] += render_samples;
		renderBufs[1] += render_samples;
	}
	for(int i = 0; i < samples; i++) {
		if(fmtoy->render_buf_l[i] > 32767) fmtoy->render_buf_l[i] = 32767;
		if(fmtoy->render_buf_l[i] < -32768) fmtoy->render_buf_l[i] = -32768;
		if(fmtoy->render_buf_r[i] > 32767) fmtoy->render_buf_r[i] = 32767;
		if(fmtoy->render_buf_r[i] < -32768) fmtoy->render_buf_r[i] = -32768;
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
