#include "tfi_file.h"

/*

$00			algo
$01			feedback

per-operator:
{
$03...		mul
$04...		detune+3
$05...		tl
$06...		rs
$07...		ar
$08...		dr
$09...		sr
$0A...		rr
$0B...		sl
$0C...		ssg
}

*/

int tfi_file_load(struct tfi_file *tfi, uint8_t *data, size_t data_len) {
	if(data_len < 42) return -1;

	uint8_t *p = data;

	tfi->alg = *p++;
	tfi->fb = *p++;

	for(int i = 0; i < 4; i++) {
		tfi->operators[i].mul = *p++;
		tfi->operators[i].dt = *p++;
		tfi->operators[i].tl = *p++;
		tfi->operators[i].rs = *p++;
		tfi->operators[i].ar = *p++;
		tfi->operators[i].dr = *p++;
		tfi->operators[i].sr = *p++;
		tfi->operators[i].rr = *p++;
		tfi->operators[i].sl = *p++;
		tfi->operators[i].ssg_eg = *p++;
	}

	return 0;
}

