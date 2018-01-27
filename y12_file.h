#ifndef Y12_H_
#define Y12_H_

#include <stdint.h>
#include <stdlib.h>

struct y12_file_operator {
	uint8_t mul_dt;
	uint8_t tl;
	uint8_t ar_rs;
	uint8_t dr_am;
	uint8_t sr;
	uint8_t rr_sl;
	uint8_t ssg;
};

struct y12_file {
	uint8_t alg;
	uint8_t fb;
	char name[16], dumper[16], game[16];
	uint8_t name_len, dumper_len, game_len;
	struct y12_file_operator operators[4];
};

int y12_file_load(struct y12_file *f, uint8_t *data, size_t data_len);

#endif /* Y12_H_ */
