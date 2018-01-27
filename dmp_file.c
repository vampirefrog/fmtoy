#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "dmp_file.h"
#include "tools.h"

int dmp_file_load(struct dmp_file *f, uint8_t *data, size_t data_len, int system) {
	memset(f, 0, sizeof(struct dmp_file));

	if(data[0] != 9) {
		fprintf(stderr, "Unsupported version 0x%02x (%d)\n", data[0], data[0]);
		return -1;
	}

	if(data_len < 51) {
		fprintf(stderr, "File too short %lu < 51\n", (unsigned long)data_len);
		return -1;
	}

	f->version = data[0];
	f->mode = data[1];

	if(f->mode == 1) {
		uint8_t *p = data + 2;
		f->num_operators = *p++ == 0 ? 2 : 4;
		f->lfo = *p++;
		f->fb = *p++;
		f->alg = *p++;
		if(system == DMP_FILE_GENESIS) {
			f->lfo2 = *p++;
		}

		for(int i = 0; i < f->num_operators; i++) {
			f->operators[i].mult = *p++;
			f->operators[i].tl   = *p++;
			f->operators[i].ar   = *p++;
			f->operators[i].dr   = *p++;
			f->operators[i].sl   = *p++;
			f->operators[i].rr   = *p++;
			f->operators[i].am   = *p++;
			if(system == DMP_FILE_GENESIS) {
				f->operators[i].ksr = *p++;
				f->operators[i].dt  = *p++;
				f->operators[i].d2r = *p++;
				f->operators[i].ssg = *p++;
			}
		}
	} else {
		fprintf(stderr, "Unsupported mode %d\n", f->mode);
		return -1;
	}

	return 0;
}
