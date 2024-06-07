#include <math.h>
#include <stdlib.h>

#include "fmtoy.h"
#include "fmtoy_ymf262.h"
#include "chips/ymf262.h"

static void fmwrite(void *ptr, uint8_t reg, uint8_t val) {
	ymf262_write(ptr, 0, reg);
	ymf262_write(ptr, 1, val);
}

static int fmtoy_ymf262_init(struct fmtoy *fmtoy, int clock, int sample_rate, struct fmtoy_channel *channel) {
	channel->chip->clock = clock;
	channel->chip->data = ymf262_init(clock*4, sample_rate);
	fmwrite(channel->chip->data, 0x01, 0x20);

	return 0;
}

static int fmtoy_ymf262_destroy(struct fmtoy *fmtoy, struct fmtoy_channel *channel) {
	return 0;
}

static void fmtoy_ymf262_program_change(struct fmtoy *fmtoy, uint8_t program, struct fmtoy_channel *channel) {
	struct opl_voice *v = &fmtoy->opl_voices[program];
	int chan_offsets[] = {
		0x00, 0x01, 0x02,
		0x08, 0x09, 0x0a,
		0x10, 0x11, 0x12,
	};
	for(int i = 0; i < 9; i++) {
		fmwrite(channel->chip->data, 0xc0 + i, v->ch_fb_cnt[0]);
		for(int j = 0; j < 2; j++) {
			struct opl_voice_operator *op = &v->operators[j];
			fmwrite(channel->chip->data, 0x20 + chan_offsets[i] + j * 3, op->am_vib_eg_ksr_mul);
			fmwrite(channel->chip->data, 0x40 + chan_offsets[i] + j * 3, op->ksl_tl);
			fmwrite(channel->chip->data, 0x60 + chan_offsets[i] + j * 3, op->ar_dr);
			fmwrite(channel->chip->data, 0x80 + chan_offsets[i] + j * 3, op->sl_rr);
			fmwrite(channel->chip->data, 0xe0 + chan_offsets[i] + j * 3, op->ws);
		}
	}
}

static void fmtoy_ymf262_set_pitch(struct fmtoy *fmtoy, int chip_channel, float pitch, struct fmtoy_channel *channel) {
	int block_fnum = opl_pitch_to_block_fnum(pitch, channel->chip->clock);
	fmwrite(channel->chip->data, 0xa0 + chip_channel, block_fnum & 0xff);
	fmwrite(channel->chip->data, 0xb0 + chip_channel, (channel->chip->channels[chip_channel].on ? 0x20 : 0x00) | block_fnum >> 8);
}

static void fmtoy_ymf262_pitch_bend(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, struct fmtoy_channel *channel) {
	fmtoy_ymf262_set_pitch(fmtoy, chip_channel, pitch, channel);
}

static void fmtoy_ymf262_note_on(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, uint8_t velocity, struct fmtoy_channel *channel) {
	fmtoy_ymf262_set_pitch(fmtoy, chip_channel, pitch, channel);
}

static void fmtoy_ymf262_note_off(struct fmtoy *fmtoy, uint8_t chip_channel, uint8_t velocity, struct fmtoy_channel *channel) {
	fmtoy_ymf262_set_pitch(fmtoy, chip_channel, channel->chip->channels[chip_channel].pitch, channel);
}

static void fmtoy_ymf262_render(struct fmtoy *fmtoy, stream_sample_t **buffers, int num_samples, struct fmtoy_channel *channel) {
	ymf262_update_one(channel->chip->data, buffers, num_samples);
}

struct fmtoy_chip fmtoy_chip_ymf262 = {
	.name = "YMF262",
	.init = fmtoy_ymf262_init,
	.destroy = fmtoy_ymf262_destroy,
	.program_change = fmtoy_ymf262_program_change,
	.pitch_bend = fmtoy_ymf262_pitch_bend,
	.note_on = fmtoy_ymf262_note_on,
	.note_off = fmtoy_ymf262_note_off,
	.render = fmtoy_ymf262_render,
	.max_poliphony = 9,
};
