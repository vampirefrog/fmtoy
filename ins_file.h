/*

================================================================================
INS (MVSTracker Instrument)
---------------------------
from "InstrEdit.cpp" and other files in MVSTracker source

$00..$03	"MVSI"
$04			Version number
$05...		Instrument Name (null-terminated string)

"v" means "varies" (since it depends on how long the instrument name is)
$v00..$v03	(mul&15) | ((dt&7) << 4)
$v04..$v07	(tl&127)
$v08..$v0B	(rs&3)<<6 | (ar&31)
$v0C..$v0F	(dr&31)
$v10..$v13	(sr&31)
$v14..$v17	(sl&15)<<4 | (rr&15)
$v18		(feedback&7)<<3 | algo&7

*/

#ifndef INS_FILE_H_
#define INS_FILE_H_

#include <stdint.h>
#include <stdlib.h>

struct ins_file_operator {
	uint8_t mul_dt;
	uint8_t tl;
	uint8_t rs_ar;
	uint8_t dr;
	uint8_t sr;
	uint8_t sl_rr;
};

struct ins_file {
	uint8_t version;
	char *name;
	int name_len, data_offset;
	struct ins_file_operator operators[4];
	uint8_t fb_alg;
};

int ins_file_load(struct ins_file *f, uint8_t *data, size_t data_len);

#endif /* INS_FILE_H_ */
