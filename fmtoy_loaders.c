#include <string.h>

#include "fmtoy_loaders.h"
#include "tools.h"
#include "opm_file.h"

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
	fmtoy_voice->sm = (opm_voice->ch_slot & 0x0f) >> 3;

	/* operators */
	int ops[4] = { 0, 2, 1, 3 };
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
