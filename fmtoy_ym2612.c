#include <math.h>

#include "fmtoy.h"
#include "fmtoy_ym2612.h"
#include "chips/fm.h"

static int fmtoy_ym2612_init(struct fmtoy *fmtoy, int clock, int sample_rate, struct fmtoy_channel *channel) {
	channel->chip->clock = clock;
	channel->chip->data = ym2612_init(0, clock, sample_rate, 0, 0);
	ym2612_reset_chip(channel->chip->data);

	// Enable 6 channel mode
	ym2612_write(channel->chip->data, 0, 0x29);
	ym2612_write(channel->chip->data, 1, 0x80);

	return 0;
}

static int fmtoy_ym2612_destroy(struct fmtoy *fmtoy, struct fmtoy_channel *channel) {
	return 0;
}

static void fmtoy_ym2612_program_change(struct fmtoy *fmtoy, uint8_t program, struct fmtoy_channel *channel) {
	struct fmtoy_opn_voice *v = &fmtoy->opn_voices[program];
	for(int i = 0; i < 6; i++) {
		int base = i < 3 ? 0 : 2;
		int c = i % 3;
		ym2612_write(channel->chip->data, base+0, 0xb0 + c);
		ym2612_write(channel->chip->data, base+1, v->fb_connect);
		ym2612_write(channel->chip->data, base+0, 0xb4 + c);
		ym2612_write(channel->chip->data, base+1, v->lr_ams_pms);
		for(int j = 0; j < 4; j++) {
			struct fmtoy_opn_voice_operator *op = &v->operators[j];
			ym2612_write(channel->chip->data, base+0, 0x30 + c + j * 4);
			ym2612_write(channel->chip->data, base+1, op->dt_mul);
			ym2612_write(channel->chip->data, base+0, 0x40 + c + j * 4);
			ym2612_write(channel->chip->data, base+1, op->tl);
			ym2612_write(channel->chip->data, base+0, 0x50 + c + j * 4);
			ym2612_write(channel->chip->data, base+1, op->ks_ar);
			ym2612_write(channel->chip->data, base+0, 0x60 + c + j * 4);
			ym2612_write(channel->chip->data, base+1, op->am_dr);
			ym2612_write(channel->chip->data, base+0, 0x70 + c + j * 4);
			ym2612_write(channel->chip->data, base+1, op->sr);
			ym2612_write(channel->chip->data, base+0, 0x80 + c + j * 4);
			ym2612_write(channel->chip->data, base+1, op->sl_rr);
			ym2612_write(channel->chip->data, base+0, 0x90 + c + j * 4);
			ym2612_write(channel->chip->data, base+1, op->ssg_eg);
		}
	}
}

static void fmtoy_ym2612_set_pitch(struct fmtoy *fmtoy, int chip_channel, float pitch, struct fmtoy_channel *channel) {
	uint8_t octave = (69 + 12 * log2(pitch / 440.0)) / 12;
	uint16_t fnum = (144 * pitch * (1 << 20) / channel->chip->clock) / (1 << (octave - 1));
	int base = chip_channel < 3 ? 0 : 2;
	chip_channel = chip_channel % 3;
	ym2612_write(channel->chip->data, base+0, 0xa4 + chip_channel);
	ym2612_write(channel->chip->data, base+1, octave << 3 | (fnum >> 8 & 0x07));
	ym2612_write(channel->chip->data, base+0, 0xa0 + chip_channel);
	ym2612_write(channel->chip->data, base+1, fnum & 0xff);
}

static void fmtoy_ym2612_pitch_bend(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, struct fmtoy_channel *channel) {
	fmtoy_ym2612_set_pitch(fmtoy, chip_channel, pitch, channel);
}

static void fmtoy_ym2612_note_on(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, uint8_t velocity, struct fmtoy_channel *channel) {
	fmtoy_ym2612_set_pitch(fmtoy, chip_channel, pitch, channel);
	chip_channel = chip_channel < 3 ? chip_channel : (chip_channel + 1);
	// ym2612_write(channel->chip->data, 0, 0x28);
	// ym2612_write(channel->chip->data, 1, chip_channel);
	ym2612_write(channel->chip->data, 0, 0x28);
	ym2612_write(channel->chip->data, 1, 0xf0 + chip_channel);
}

static void fmtoy_ym2612_note_off(struct fmtoy *fmtoy, uint8_t chip_channel, uint8_t velocity, struct fmtoy_channel *channel) {
	chip_channel = chip_channel < 3 ? chip_channel : chip_channel + 1;
	ym2612_write(channel->chip->data, 0, 0x28);
	ym2612_write(channel->chip->data, 1, chip_channel);
}

static void fmtoy_ym2612_render(struct fmtoy *fmtoy, stream_sample_t **buffers, int num_samples, struct fmtoy_channel *channel) {
	ym2612_update_one(channel->chip->data, buffers, num_samples);
}

struct fmtoy_chip fmtoy_chip_ym2612 = {
	.name = "YM2612",
	.init = fmtoy_ym2612_init,
	.destroy = fmtoy_ym2612_destroy,
	.program_change = fmtoy_ym2612_program_change,
	.pitch_bend = fmtoy_ym2612_pitch_bend,
	.note_on = fmtoy_ym2612_note_on,
	.note_off = fmtoy_ym2612_note_off,
	.render = fmtoy_ym2612_render,
	.max_poliphony = 6,
};
