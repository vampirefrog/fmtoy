#ifndef OPM_FILE_H_
#define OPM_FILE_H_

#include <stdint.h>
#include <stdlib.h>

struct opm_file_operator {
	uint8_t ar, d1r, d2r, rr, d1l, tl, ks, mul, dt1, dt2, ame;
};

struct opm_file_voice {
	uint8_t used;
	char name[256];
	uint8_t lfo_lfrq, lfo_amd, lfo_pmd, lfo_wf, nfrq;
	uint8_t ch_pan, ch_fl, ch_con, ch_ams, ch_pms, ch_slot, ch_ne;
	struct opm_file_operator operators[4];
};

#define OPM_FILE_MAX_VOICES 256
struct opm_file {
	struct opm_file_voice voices[OPM_FILE_MAX_VOICES];
};

int opm_file_load(struct opm_file *f, uint8_t *data, size_t data_len);

#endif /* OPM_FILE_H_ */
