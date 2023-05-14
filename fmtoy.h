#ifndef FMTOY_H_
#define FMTOY_H_

#include <stdint.h>

#include "fmtoy_voice.h"

#include "chips/mamedef.h"
#include "chips/ym2151.h"
#include "chips/fm.h"

// used for tracking poliphony
struct fmtoy_chip_channel {
	uint8_t on, note;
	float pitch;
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
	void (*mod_wheel)(struct fmtoy *, int mod, struct fmtoy_channel *);
	void (*note_on)(struct fmtoy *, uint8_t chip_channel, float pitch, uint8_t velocity, struct fmtoy_channel *);
	void (*note_off)(struct fmtoy *, uint8_t chip_channel, uint8_t velocity, struct fmtoy_channel *);
	void (*render)(struct fmtoy *, stream_sample_t **buffers, int num_samples, struct fmtoy_channel *);
	int clock, max_poliphony;

	struct fmtoy_chip_channel channels[9]; // preallocate channels for poliphony, though we won't always use all of them
};

struct fmtoy_channel {
	struct fmtoy_chip *chip;
	int program;
	int pitch_bend;
	uint8_t con, op_mask, tl[4]; // used for velocity
};

struct fmtoy {
	stream_sample_t *render_buf_l, *render_buf_r;
	stream_sample_t *chip_buf_l, *chip_buf_r;
	int sample_rate, buf_size;

	// voices
	int num_voices;
	struct fmtoy_opl_voice opl_voices[128];
	struct fmtoy_opm_voice opm_voices[128];
	struct fmtoy_opn_voice opn_voices[128];

	struct fmtoy_channel channels[16];

	int lfo_clock_phase, lfo_clock_period;
	int clock; // chip clock
	int pitch_bend_range; // in semitones
};

struct fmtoy *fmtoy_new(int clock, int sample_rate);
void fmtoy_init(struct fmtoy *fmtoy, int clock, int sample_rate);
void fmtoy_destroy(struct fmtoy *fmtoy);
void fmtoy_load_opm_voice(struct fmtoy *fmtoy, int voice_num, struct fmtoy_opm_voice *voice);
void fmtoy_append_opm_voice(struct fmtoy *fmtoy, struct fmtoy_opm_voice *voice);
void fmtoy_append_opl_voice(struct fmtoy *fmtoy, struct fmtoy_opl_voice *voice);
void fmtoy_opm_voice_to_fmtoy_opn_voice(struct fmtoy_opm_voice *opmv, struct fmtoy_opn_voice *opnv);
void fmtoy_note_on(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity);
void fmtoy_note_off(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity);
void fmtoy_render(struct fmtoy *fmtoy, int samples);
stream_sample_t *fmtoy_get_buf_l(struct fmtoy *fmtoy);
stream_sample_t *fmtoy_get_buf_r(struct fmtoy *fmtoy);
void fmtoy_program_change(struct fmtoy *fmtoy, uint8_t channel, uint8_t program);
void fmtoy_pitch_bend(struct fmtoy *fmtoy, uint8_t channel, int bend);
void fmtoy_cc(struct fmtoy *fmtoy, uint8_t channel, int cc, int value);
const char *fmtoy_channel_name(struct fmtoy *fmtoy, uint8_t channel);

#endif /* FMTOY_H_ */
