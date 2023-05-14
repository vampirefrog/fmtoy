#include <stdlib.h>
#include <string.h>
#ifndef __EMSCRIPTEN__
#include <stdio.h>
#endif

#include "fmtoy_voice.h"

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
