#ifndef FMTOY_LOADERS_H_
#define FMTOY_LOADERS_H_

/* Separate loading routines */
/* This file can be omitted when linking, if you don't need any I/O */

#include "fmtoy.h"
#include "opm_file.h"

void fmtoy_load_voice_file(struct fmtoy *fmtoy, char *filename);
void fmtoy_opm_voice_load_opm_file_voice(struct fmtoy_opm_voice *fmtoy_voice, struct opm_file_voice *opm_voice);

#endif /* FMTOY_LOADERS_H_ */
