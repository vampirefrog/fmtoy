#include <string.h>

#include "y12_file.h"

int y12_file_load(struct y12_file *f, uint8_t *data, size_t data_len) {
	if(data_len < 0x80) return -1;

	uint8_t *p = data;
	for(int i = 0; i < 4; i++) {
		f->operators[i].mul_dt = *p++;
		f->operators[i].tl = *p++;
		f->operators[i].ar_rs = *p++;
		f->operators[i].dr_am = *p++;
		f->operators[i].sr = *p++;
		f->operators[i].rr_sl = *p++;
		f->operators[i].ssg = *p++;
		p += 9;
	}
	f->alg = *p++;
	f->fb = *p++;
	p += 14;
	memcpy(f->name, p, 16);
	f->name_len = strnlen(f->name, 16);
	p += 16;
	memcpy(f->dumper, p, 16);
	f->dumper_len = strnlen(f->dumper, 16);
	p += 16;
	memcpy(f->game, p, 16);
	f->game_len = strnlen(f->game, 16);

	return 0;
}
