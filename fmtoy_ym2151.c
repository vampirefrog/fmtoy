#include <math.h>
#include "fmtoy.h"
#include "fmtoy_ym2151.h"
#include "tools.h"

#include "libvgm/emu/EmuStructs.h"
#include "libvgm/emu/SoundEmu.h"
#include "libvgm/emu/SoundDevs.h"
#include "libvgm/emu/EmuCores.h"
#include "libvgm/emu/cores/ym2151.h"

static int fmtoy_ym2151_init(struct fmtoy *fmtoy, int clock, int sample_rate, struct fmtoy_channel *channel) {
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

	if(SndEmu_Start(DEVID_YM2151, &devCfg, devinf))
		return -2;

	devinf->devDef->Reset(devinf->dataPtr);

	return 0;
}

static int fmtoy_ym2151_destroy(struct fmtoy *fmtoy, struct fmtoy_channel *channel) {
	SndEmu_Stop(channel->chip->data);
	free(channel->chip->data);
	return 0;
}

static void fmtoy_ym2151_program_change(struct fmtoy *fmtoy, uint8_t program, struct fmtoy_channel *channel) {
	DEVFUNC_WRITE_A8D8 writefn;
	DEV_INFO *devinf = channel->chip->data;
	SndEmu_GetDeviceFunc(devinf->devDef, RWF_REGISTER | RWF_QUICKWRITE, DEVRW_A8D8, 0, (void**)&writefn);
	struct opm_voice *v = &fmtoy->opm_voices[program];

	// prepare for velocity
	channel->con = v->rl_fb_con & 0x07;
	channel->op_mask = v->slot;
	for(int j = 0; j < 4; j++)
		channel->tl[j] = v->operators[j].tl;

	writefn(devinf->dataPtr, 0x18, v->lfrq);
	writefn(devinf->dataPtr, 0x0f, v->ne_nfrq);
	writefn(devinf->dataPtr, 0x1b, v->w);
	for(int i = 0; i < 8; i++) {
		writefn(devinf->dataPtr, 0x20 + i, v->rl_fb_con);
		writefn(devinf->dataPtr, 0x38 + i, v->pms_ams);
		for(int j = 0; j < 4; j++) {
			struct opm_voice_operator *op = &v->operators[j];
			writefn(devinf->dataPtr, 0x40 + i + j * 8, op->dt1_mul);
			writefn(devinf->dataPtr, 0x60 + i + j * 8, op->tl);
			writefn(devinf->dataPtr, 0x80 + i + j * 8, op->ks_ar);
			writefn(devinf->dataPtr, 0xa0 + i + j * 8, op->ams_d1r);
			writefn(devinf->dataPtr, 0xc0 + i + j * 8, op->dt2_d2r);
			writefn(devinf->dataPtr, 0xe0 + i + j * 8, op->d1l_rr);
		}
	}
}

static void fmtoy_ym2151_set_pitch(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, struct fmtoy_channel *channel) {
	DEVFUNC_WRITE_A8D8 writefn;
	DEV_INFO *devinf = channel->chip->data;
	SndEmu_GetDeviceFunc(devinf->devDef, RWF_REGISTER | RWF_QUICKWRITE, DEVRW_A8D8, 0, (void**)&writefn);
	int kc_kf = opm_pitch_to_kc_kf(pitch, channel->chip->clock);
	writefn(devinf->dataPtr, 0x28 + chip_channel, kc_kf >> 8);
	writefn(devinf->dataPtr, 0x30 + chip_channel, kc_kf & 0xff);
}

static void fmtoy_ym2151_note_on(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, uint8_t velocity, struct fmtoy_channel *channel) {
	DEVFUNC_WRITE_A8D8 writefn;
	DEV_INFO *devinf = channel->chip->data;
	SndEmu_GetDeviceFunc(devinf->devDef, RWF_REGISTER | RWF_QUICKWRITE, DEVRW_A8D8, 0, (void**)&writefn);

	fmtoy_ym2151_set_pitch(fmtoy, chip_channel, pitch, channel);

	// set velocity
	int tl = (127 - velocity) / 4;
	// C2
	if(channel->op_mask & 0x08)
		writefn(devinf->dataPtr, 0x78 + chip_channel, MIN(127, channel->tl[3] + tl));
	// C1
	if((channel->op_mask & 0x02) && channel->con >= 4)
		writefn(devinf->dataPtr, 0x70 + chip_channel, MIN(127, channel->tl[2] + tl));
	// M2
	if((channel->op_mask & 0x04) && channel->con >= 5)
		writefn(devinf->dataPtr, 0x68 + chip_channel, MIN(127, channel->tl[1] + tl));
	// M1
	if((channel->op_mask & 0x01) && channel->con >= 7)
		writefn(devinf->dataPtr, 0x60 + chip_channel, MIN(127, channel->tl[0] + tl));

	// key on
	writefn(devinf->dataPtr, 0x08, channel->op_mask << 3 | (chip_channel & 0x07));
}

static void fmtoy_ym2151_note_off(struct fmtoy *fmtoy, uint8_t chip_channel, uint8_t velocity, struct fmtoy_channel *channel) {
	DEVFUNC_WRITE_A8D8 writefn;
	DEV_INFO *devinf = channel->chip->data;
	SndEmu_GetDeviceFunc(devinf->devDef, RWF_REGISTER | RWF_QUICKWRITE, DEVRW_A8D8, 0, (void**)&writefn);
	writefn(devinf->dataPtr, 0x08, chip_channel & 0x07);
}

static void fmtoy_ym2151_pitch_bend(struct fmtoy *fmtoy, uint8_t chip_channel, float pitch, struct fmtoy_channel *channel) {
	fmtoy_ym2151_set_pitch(fmtoy, chip_channel, pitch, channel);
}

static void fmtoy_ym2151_mod_wheel(struct fmtoy *fmtoy, int mod, struct fmtoy_channel *channel) {
	DEVFUNC_WRITE_A8D8 writefn;
	DEV_INFO *devinf = channel->chip->data;
	SndEmu_GetDeviceFunc(devinf->devDef, RWF_REGISTER | RWF_QUICKWRITE, DEVRW_A8D8, 0, (void**)&writefn);
	struct opm_voice *v = &fmtoy->opm_voices[channel->program];
	writefn(devinf->dataPtr, 0x19, ((mod * v->pmd / 127) & 0x7f) | 0x80);
}

static void fmtoy_ym2151_render(struct fmtoy *fmtoy, stream_sample_t **buffers, int num_samples, struct fmtoy_channel *channel) {
	DEV_INFO *devinf = channel->chip->data;
	devinf->devDef->Update(devinf->dataPtr, num_samples, buffers);
}

struct fmtoy_chip fmtoy_chip_ym2151 = {
	.name = "YM2151",
	.init = fmtoy_ym2151_init,
	.destroy = fmtoy_ym2151_destroy,
	.program_change = fmtoy_ym2151_program_change,
	.pitch_bend = fmtoy_ym2151_pitch_bend,
	.mod_wheel = fmtoy_ym2151_mod_wheel,
	.note_on = fmtoy_ym2151_note_on,
	.note_off = fmtoy_ym2151_note_off,
	.render = fmtoy_ym2151_render,
	.max_poliphony = 8,
};
