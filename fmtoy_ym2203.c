#include <math.h>

#include "fmtoy.h"
#include "fmtoy_ym2203.h"
#include "chips/fm.h"

static void fmtoy_ym2203_set_pitch(struct fmtoy *fmtoy, int chip_channel, float pitch, struct fmtoy_channel *channel) {
	uint8_t octave = (69 + 12 * log2(pitch / 440.0)) / 12 - 1;
	uint16_t fnum = (144 * pitch * (1 << 19) / channel->chip->clock) / (1 << (octave - 1));
	ym2203_write(channel->chip->data, 0, 0xa4 + chip_channel);
	ym2203_write(channel->chip->data, 1, octave << 3 | (fnum >> 8 & 0x07));
	ym2203_write(channel->chip->data, 0, 0xa0 + chip_channel);
	ym2203_write(channel->chip->data, 1, fnum & 0xff);
}

static int fmtoy_ym2203_init(struct fmtoy *fmtoy, int sample_rate, struct fmtoy_channel *channel) {
	channel->chip->clock = 3579545;
	channel->chip->data = ym2203_init(0, channel->chip->clock, sample_rate, 0, 0, 0);
	ym2203_reset_chip(channel->chip->data);

	return 0;
}

static int fmtoy_ym2203_destroy(struct fmtoy *fmtoy, struct fmtoy_channel *channel) {
	return 0;
}

static void fmtoy_ym2203_program_change(struct fmtoy *fmtoy, uint8_t program, struct fmtoy_channel *channel) {
	struct fmtoy_opn_voice *v = &fmtoy->opn_voices[program];
	for(int i = 0; i < 3; i++) {
		ym2203_write(channel->chip->data, 0, 0xb0 + i);
		ym2203_write(channel->chip->data, 1, v->fb_connect);
		ym2203_write(channel->chip->data, 0, 0xb4 + i);
		ym2203_write(channel->chip->data, 1, v->lr_ams_pms);
		for(int j = 0; j < 4; j++) {
			struct fmtoy_opn_voice_operator *op = &v->operators[j];
			ym2203_write(channel->chip->data, 0, 0x30 + i + j * 4);
			ym2203_write(channel->chip->data, 1, op->dt_multi);
			ym2203_write(channel->chip->data, 0, 0x40 + i + j * 4);
			ym2203_write(channel->chip->data, 1, op->tl);
			ym2203_write(channel->chip->data, 0, 0x50 + i + j * 4);
			ym2203_write(channel->chip->data, 1, op->ks_ar);
			ym2203_write(channel->chip->data, 0, 0x60 + i + j * 4);
			ym2203_write(channel->chip->data, 1, op->am_dr);
			ym2203_write(channel->chip->data, 0, 0x70 + i + j * 4);
			ym2203_write(channel->chip->data, 1, op->sr);
			ym2203_write(channel->chip->data, 0, 0x80 + i + j * 4);
			ym2203_write(channel->chip->data, 1, op->sl_rr);
			ym2203_write(channel->chip->data, 0, 0x90 + i + j * 4);
			ym2203_write(channel->chip->data, 1, op->ssg_eg);
		}
	}
}

static void fmtoy_ym2203_pitch_bend(struct fmtoy *fmtoy, int chip_channel, float pitch, struct fmtoy_channel *channel) {
	fmtoy_ym2203_set_pitch(fmtoy, chip_channel, pitch, channel);
}

static void fmtoy_ym2203_note_on(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, uint8_t velocity, struct fmtoy_channel *channel) {
	fmtoy_ym2203_set_pitch(fmtoy, chip_channel, pitch, channel);
	// ym2203_write(channel->chip->data, 0, 0x28);
	// ym2203_write(channel->chip->data, 1, chip_channel);
	ym2203_write(channel->chip->data, 0, 0x28);
	ym2203_write(channel->chip->data, 1, 0xf0 + chip_channel);
}

static void fmtoy_ym2203_note_off(struct fmtoy *fmtoy, uint8_t chip_channel, uint8_t velocity, struct fmtoy_channel *channel) {
	ym2203_write(channel->chip->data, 0, 0x28);
	ym2203_write(channel->chip->data, 1, chip_channel);
}

static void fmtoy_ym2203_render(struct fmtoy *fmtoy, stream_sample_t **buffers, int num_samples, struct fmtoy_channel *channel) {
	ym2203_update_one(channel->chip->data, buffers, num_samples);
}

struct fmtoy_chip fmtoy_chip_ym2203 = {
	.name = "YM2203",
	.init = fmtoy_ym2203_init,
	.destroy = fmtoy_ym2203_destroy,
	.program_change = fmtoy_ym2203_program_change,
	.pitch_bend = fmtoy_ym2203_pitch_bend,
	.note_on = fmtoy_ym2203_note_on,
	.note_off = fmtoy_ym2203_note_off,
	.render = fmtoy_ym2203_render,
	.max_poliphony = 3,
};
