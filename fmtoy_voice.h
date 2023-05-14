#pragma once

#include <stdint.h>

struct fmtoy_opm_voice_operator {
	uint8_t dt1_mul, tl, ks_ar, ame_d1r, dt2_d2r, d1l_rr;
};

struct fmtoy_opm_voice {
	char name[256];
	// per chip registers - LFO and noise
	uint8_t lfrq, pmd, amd, w, ne_nfrq;
	// per channel registers
	uint8_t rl_fb_con, pms_ams;
	// slot mask, bits are in mask order: 0 2 1 3 - M1 C1 M2 C2
	uint8_t sm;
	// operators, in chip order: 0 1 2 3 - M1 M2 C1 C2
	struct fmtoy_opm_voice_operator operators[4];
};

struct fmtoy_opn_voice_operator {
	uint8_t dt_mul, tl, ks_ar, am_dr, sr, sl_rr, ssg_eg;
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

struct fmtoy_opl_voice_operator {
	uint8_t am_vib_eg_ksr_mul, ksl_tl, ar_dr, sl_rr, ws;
};

struct fmtoy_opl_voice {
	char name[256];
	// per chip registers
	uint8_t am_vib;
	// per channel registers
	uint8_t fb_con;
	// operators
	struct fmtoy_opl_voice_operator operators[2];
};

struct fmtoy_opm_voice *fmtoy_opm_voice_new();
int fmtoy_opm_voice_free(struct fmtoy_opm_voice *voice);
void fmtoy_opm_voice_init(struct fmtoy_opm_voice *voice);
#ifndef __EMSCRIPTEN__
void fmtoy_opm_voice_dump(struct fmtoy_opm_voice *voice);
#endif
void fmtoy_opm_voice_set_lfo(struct fmtoy_opm_voice *voice, uint8_t lfrq, uint8_t pmd, uint8_t amd, uint8_t w);
void fmtoy_opm_voice_set_noise(struct fmtoy_opm_voice *voice, uint8_t ne, uint8_t nfrq);
void fmtoy_opm_voice_set_pan(struct fmtoy_opm_voice *voice, uint8_t rl);
void fmtoy_opm_voice_set_feedback(struct fmtoy_opm_voice *voice, uint8_t fb);
void fmtoy_opm_voice_set_connection(struct fmtoy_opm_voice *voice, uint8_t con);
void fmtoy_opm_voice_set_pms_ams(struct fmtoy_opm_voice *voice, uint8_t pms, uint8_t ams);
void fmtoy_opm_voice_set_slot_mask(struct fmtoy_opm_voice *voice, uint8_t sm);
void fmtoy_opm_voice_set_operator_dt1_mul(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t dt, uint8_t mul);
void fmtoy_opm_voice_set_operator_tl(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t tl);
void fmtoy_opm_voice_set_operator_ks_ar(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t ks, uint8_t ar);
void fmtoy_opm_voice_set_operator_ame_d1r(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t ame, uint8_t d1r);
void fmtoy_opm_voice_set_operator_dt2_d2r(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t dt2, uint8_t d2r);
void fmtoy_opm_voice_set_operator_d1l_rr(struct fmtoy_opm_voice *voice, uint8_t op, uint8_t d1l, uint8_t rr);

void fmtoy_opm_voice_to_fmtoy_opn_voice(struct fmtoy_opm_voice *opmv, struct fmtoy_opn_voice *opnv);
