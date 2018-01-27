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
	// slot mask
	uint8_t sm;
	// operators, in chip order M1 M2 C1 C2
	struct fmtoy_opm_voice_operator operators[4];
};

struct fmtoy_opn_voice_operator {
	uint8_t dt_multi, tl, ks_ar, am_dr, sr, sl_rr, ssg_eg;
};

struct fmtoy_opn_voice {
	char name[256];
	uint8_t lfo, fb_connect, lr_ams_pms;
	struct fmtoy_opn_voice_operator operators[4];
};

// used for tracking poliphony
struct fmtoy_chip_channel {
	uint8_t on, note;
	uint32_t frames;
};

struct fmtoy {
	stream_sample_t *render_buf_l, *render_buf_r;
	stream_sample_t *chip_buf_l, *chip_buf_r;
	int sample_rate, buf_size;

	// voices
	int num_voices;
	struct fmtoy_opm_voice opm_voices[128];
	struct fmtoy_opn_voice opn_voices[128];

	// YM2151 OPM
	void *ym2151;
	struct fmtoy_chip_channel ym2151_channels[8];

	// YM2203 OPN
	void *ym2203;
	struct fmtoy_chip_channel ym2203_channels[3];

	// YM2608 OPNA
	void *ym2608;
	struct fmtoy_chip_channel ym2608_channels[6];

	// YM2610 OPNB
	void *ym2610;
	struct fmtoy_chip_channel ym2610_channels[4];

	// YM2612 OPN2
	void *ym2612;
	struct fmtoy_chip_channel ym2612_channels[6];
};

void fmtoy_init(struct fmtoy *fmtoy, int sample_rate);
void fmtoy_load_voice(struct fmtoy *fmtoy, char *filename);
void fmtoy_note_on(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity);
void fmtoy_note_off(struct fmtoy *fmtoy, uint8_t channel, uint8_t note, uint8_t velocity);
void fmtoy_render(struct fmtoy *fmtoy, int samples);
void fmtoy_program_change(struct fmtoy *fmtoy, uint8_t channel, uint8_t program);
void fmtoy_pitch_bend(struct fmtoy *fmtoy, uint8_t channel, int bend);
void fmtoy_cc(struct fmtoy *fmtoy, uint8_t channel, int cc, int value);

#endif /* FMTOY_H_ */
