#include <string.h>
#include <stdio.h>

#include "fmtoy_loaders.h"
#include "tools.h"

void fmtoy_load_voice_file(struct fmtoy *fmtoy, char *filename) {
	int l = strlen(filename);
	if(!strncasecmp(filename + l - 4, ".opm", 4)) {
		struct opm_file opm;
		size_t l;
		uint8_t *data = load_file(filename, &l);
		opm_file_load(&opm, data, l);
		for(int i = 0; i < OPM_FILE_MAX_VOICES; i++) {
			if(!opm.voices[i].used) continue;
			if(fmtoy->num_voices > 127) continue;
			struct fmtoy_opm_voice opmv;
			fmtoy_opm_voice_load_opm_file_voice(&opmv, &opm.voices[i]);
			fmtoy_append_opm_voice(fmtoy, &opmv);
		}
	} else if(!strncasecmp(filename + l - 4, ".bnk", 4)) {
		struct bnk_file bnk;
		bnk_file_init(&bnk);
		size_t l;
		uint8_t *data = load_file(filename, &l);
		int r = bnk_file_load(&bnk, data, l);
		if(r < 0) {
			fprintf(stderr, "Could not load bnk file %s\n", filename);
			return;
		}
		for(int i = 0; i < bnk.num_instruments; i++) {
			struct fmtoy_opl_voice oplv;
			if(!(bnk.names[i].flags & 0x01)) continue;
			fmtoy_opl_voice_load_bnk_file_voice(&oplv, &bnk.names[i], &bnk.instruments[bnk.names[i].index]);
			fmtoy_append_opl_voice(fmtoy, &oplv);
		}
	}
}

void fmtoy_opm_voice_load_opm_file_voice(struct fmtoy_opm_voice *fmtoy_voice, struct opm_file_voice *opm_voice) {
	/* per chip registers */
	fmtoy_voice->lfrq = opm_voice->lfo_lfrq;
	fmtoy_voice->amd = opm_voice->lfo_amd & 0x7f;
	fmtoy_voice->pmd = opm_voice->lfo_pmd & 0x7f;
	fmtoy_voice->w = opm_voice->lfo_wf;
	fmtoy_voice->ne_nfrq = (opm_voice->ch_ne ? 0x80 : 0x00) | (opm_voice->nfrq & 0x1f);

	/* per channel registers */
	fmtoy_voice->rl_fb_con = 3 << 6 | (opm_voice->ch_fl & 0x07) << 3 | (opm_voice->ch_con & 0x07);
	fmtoy_voice->pms_ams = (opm_voice->ch_pms & 0x07) << 4 | (opm_voice->ch_ams & 0x03);

	/* slot mask */
	fmtoy_voice->sm = opm_voice->ch_slot & 0x78;

	/* operators */
	int ops[4] = { 0, 1, 2, 3 };
	for(int j = 0; j < 4; j++) {
		struct fmtoy_opm_voice_operator *fop = &fmtoy_voice->operators[j];
		struct opm_file_operator *op = &opm_voice->operators[ops[j]];
		fop->dt1_mul = op->dt1 << 4 | op->mul;
		fop->tl = op->tl;
		fop->ks_ar = op->ks << 6 | op->ar;
		fop->ame_d1r = op->ame << 7 | op->d1r;
		fop->dt2_d2r = op->dt2 << 6 | op->d2r;
		fop->d1l_rr = op->d1l << 4 | op->rr;
	}
}

void fmtoy_opl_voice_load_bnk_file_voice(struct fmtoy_opl_voice *fmtoy_voice, struct bnk_file_name *name, struct bnk_file_instrument *instr) {
	memset(fmtoy_voice->name, 0, sizeof(fmtoy_voice->name));
	memcpy(fmtoy_voice->name, name->name, 8);

	/* per chip registers */
	fmtoy_voice->am_vib = 0;
	fmtoy_voice->fb_con = instr->operators[0].fb << 1 | (!instr->operators[0].con);
	for(int i = 0; i < 2; i++) {
		struct fmtoy_opl_voice_operator *fop = &fmtoy_voice->operators[i];
		struct bnk_file_operator *op = &instr->operators[i];
		fop->am_vib_eg_ksr_mul = (op->am & 1) << 7 | (op->vib & 1) << 6 | (op->eg & 1) << 5 | op->ksr << 4 | (op->mul & 0x0f);
		fop->ksl_tl = (op->ksl & 0x03) << 6 | (op->tl & 0x3f);
		fop->ar_dr = op->ar << 4 | (op->dr & 0x0f);
		fop->sl_rr = op->sl << 4 | (op->rr & 0x0f);
		fop->ws = op->wave_sel & 0x03;
	}
}
