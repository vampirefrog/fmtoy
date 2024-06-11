#include "fmtoy.h"
#include "fmtoy_ym2203.h"
#include "libvgm/emu/SoundEmu.h"
#include "libvgm/emu/SoundDevs.h"

static int fmtoy_ym2203_init(struct fmtoy *fmtoy, int clock, int sample_rate, struct fmtoy_channel *channel) {
	channel->chip->clock = clock;
	DEV_GEN_CFG devCfg = {
		.emuCore = 0,
		.srMode = DEVRI_SRMODE_NATIVE,
		.flags = 0x00,
		.clock = clock,
		.smplRate = sample_rate,
	};
	DEV_INFO *devinf = malloc(sizeof(DEV_INFO));
	if(!devinf) return -1;
	channel->chip->data = devinf;

	if(SndEmu_Start(DEVID_YM2203, &devCfg, devinf))
		return -2;

	devinf->devDef->Reset(devinf->dataPtr);

	return 0;
}

static int fmtoy_ym2203_destroy(struct fmtoy *fmtoy, struct fmtoy_channel *channel) {
	SndEmu_Stop(channel->chip->data);
	free(channel->chip->data);
	return 0;
}

static void fmtoy_ym2203_program_change(struct fmtoy *fmtoy, uint8_t program, struct fmtoy_channel *channel) {
	DEVFUNC_WRITE_A8D8 writefn;
	DEV_INFO *devinf = channel->chip->data;
	SndEmu_GetDeviceFunc(devinf->devDef, RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void**)&writefn);
	struct opn_voice *v = &fmtoy->opn_voices[program];
	for(int i = 0; i < 3; i++) {
		writefn(devinf->dataPtr, 0, 0xb0 + i);
		writefn(devinf->dataPtr, 1, v->fb_con);
		writefn(devinf->dataPtr, 0, 0xb4 + i);
		writefn(devinf->dataPtr, 1, v->lr_ams_pms);
		for(int j = 0; j < 4; j++) {
			struct opn_voice_operator *op = &v->operators[j];
			writefn(devinf->dataPtr, 0, 0x30 + i + j * 4);
			writefn(devinf->dataPtr, 1, op->dt_mul);
			writefn(devinf->dataPtr, 0, 0x40 + i + j * 4);
			writefn(devinf->dataPtr, 1, op->tl);
			writefn(devinf->dataPtr, 0, 0x50 + i + j * 4);
			writefn(devinf->dataPtr, 1, op->ks_ar);
			writefn(devinf->dataPtr, 0, 0x60 + i + j * 4);
			writefn(devinf->dataPtr, 1, op->am_dr);
			writefn(devinf->dataPtr, 0, 0x70 + i + j * 4);
			writefn(devinf->dataPtr, 1, op->sr);
			writefn(devinf->dataPtr, 0, 0x80 + i + j * 4);
			writefn(devinf->dataPtr, 1, op->sl_rr);
			writefn(devinf->dataPtr, 0, 0x90 + i + j * 4);
			writefn(devinf->dataPtr, 1, op->ssg_eg);
		}
	}
}

static void fmtoy_ym2203_set_pitch(struct fmtoy *fmtoy, int chip_channel, float pitch, struct fmtoy_channel *channel) {
	DEVFUNC_WRITE_A8D8 writefn;
	DEV_INFO *devinf = channel->chip->data;
	SndEmu_GetDeviceFunc(devinf->devDef, RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void**)&writefn);
	int block_fnum = opn_pitch_to_block_fnum(pitch, channel->chip->clock);
	writefn(devinf->dataPtr, 0, 0xa4 + chip_channel);
	writefn(devinf->dataPtr, 1, block_fnum >> 8);
	writefn(devinf->dataPtr, 0, 0xa0 + chip_channel);
	writefn(devinf->dataPtr, 1, block_fnum & 0xff);
}

static void fmtoy_ym2203_pitch_bend(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, struct fmtoy_channel *channel) {
	fmtoy_ym2203_set_pitch(fmtoy, chip_channel, pitch, channel);
}

static void fmtoy_ym2203_note_on(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, uint8_t velocity, struct fmtoy_channel *channel) {
	DEVFUNC_WRITE_A8D8 writefn;
	DEV_INFO *devinf = channel->chip->data;
	SndEmu_GetDeviceFunc(devinf->devDef, RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void**)&writefn);
	fmtoy_ym2203_set_pitch(fmtoy, chip_channel, pitch, channel);
	writefn(devinf->dataPtr, 0, 0x28);
	writefn(devinf->dataPtr, 1, 0xf0 + chip_channel);
}

static void fmtoy_ym2203_note_off(struct fmtoy *fmtoy, uint8_t chip_channel, uint8_t velocity, struct fmtoy_channel *channel) {
	DEVFUNC_WRITE_A8D8 writefn;
	DEV_INFO *devinf = channel->chip->data;
	SndEmu_GetDeviceFunc(devinf->devDef, RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void**)&writefn);
	writefn(devinf->dataPtr, 0, 0x28);
	writefn(devinf->dataPtr, 1, chip_channel);
}

static void fmtoy_ym2203_render(struct fmtoy *fmtoy, stream_sample_t **buffers, int num_samples, struct fmtoy_channel *channel) {
	DEV_INFO *devinf = channel->chip->data;
	devinf->devDef->Update(devinf->dataPtr, num_samples, buffers);
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
