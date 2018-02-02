#ifndef FMTOY_H_
#define FMTOY_H_

#include <stdint.h>

#include "chips/mamedef.h"
#include "chips/ym2151.h"
#include "chips/fm.h"

struct fmtoy_opm_voice_operator {
	uint8_t dt1_mul, tl, ks_ar, ams_en_d1r, dt2_d2r, d1l_rr;
};

struct fmtoy_opm_voice {
	char name[256];
	// per chip registers - LFO and noise
	uint8_t lfrq, pmd_amd, w, ne_nfrq;
	// per channel registers
	uint8_t rl_fb_con, pms_ams;
	// slot mask, bits are in mask order: 0 2 1 3 - M1 C1 M2 C2
	uint8_t sm;
	// operators, in chip order: 0 1 2 3 - M1 M2 C1 C2
	struct fmtoy_opm_voice_operator operators[4];
};

struct fmtoy_opn_voice_operator {
	uint8_t dt_multi, tl, ks_ar, am_dr, sr, sl_rr, ssg_eg;
};

struct fmtoy_opn_voice {
	char name[256];
	// per chip registers
	uint8_t lfo;
	// per channel registers
	uint8_t fb_connect, lr_ams_pms;
	// slot mask, in mask order: 0 2 1 3 - M1 C1 M2 C2
	uint8_t sm;
	// operators, in chip order: 0 1 2 3 - M1 M2 C1 C2
	struct fmtoy_opn_voice_operator operators[4];
};

// used for tracking poliphony
struct fmtoy_chip_channel {
	uint8_t on, note;
	uint32_t frames;
};

struct fmtoy;
struct fmtoy_channel;
struct fmtoy_chip {
	const char *name;
	void *data;
	int (*init)(struct fmtoy *, int clock, int sample_rate, struct fmtoy_channel *);
	int (*destroy)(struct fmtoy *, struct fmtoy_channel *);
	void (*program_change)(struct fmtoy *, uint8_t program, struct fmtoy_channel *);
	void (*pitch_bend)(struct fmtoy *, uint8_t chip_channel, float pitch, struct fmtoy_channel *);
	void (*note_on)(struct fmtoy *, uint8_t chip_channel, float pitch, uint8_t velocity, struct fmtoy_channel *);
	void (*note_off)(struct fmtoy *, uint8_t chip_channel, uint8_t velocity, struct fmtoy_channel *);
	void (*render)(struct fmtoy *, stream_sample_t **buffers, int num_samples, struct fmtoy_channel *);
	int clock, max_poliphony;

	struct fmtoy_chip_channel channels[8]; // preallocate channels for poliphony, though we won't always use all of them
};

struct fmtoy_channel {
	struct fmtoy_chip *chip;
	int pitch_bend;
};

struct fmtoy {
	stream_sample_t *render_buf_l, *render_buf_r;
	stream_sample_t *chip_buf_l, *chip_buf_r;
	int sample_rate, buf_size;

	// voices
	int num_voices;
	struct fmtoy_opm_voice opm_voices[128];
	struct fmtoy_opn_voice opn_voices[128];

	struct fmtoy_channel channels[16];

	int clock; // chip clock
	int pitch_bend_range; // in semitones
};

void fmtoy_init(struct fmtoy *fmtoy, int clock, int sample_rate);
void fmtoy_load_voice(struct fmtoy *fmtoy, char *filename);
void fmtoy_note_on(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity);
void fmtoy_note_off(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity);
void fmtoy_render(struct fmtoy *fmtoy, int samples);
void fmtoy_program_change(struct fmtoy *fmtoy, uint8_t channel, uint8_t program);
void fmtoy_pitch_bend(struct fmtoy *fmtoy, uint8_t channel, int bend);
void fmtoy_cc(struct fmtoy *fmtoy, uint8_t channel, int cc, int value);
const char *fmtoy_channel_name(struct fmtoy *fmtoy, uint8_t channel);

#endif /* FMTOY_H_ */
