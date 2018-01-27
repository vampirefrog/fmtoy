#include <stdint.h>
#include <stdio.h>
#include "dmp_file.h"
#include "tools.h"
#include "md5.h"

int opt_system = DMP_FILE_GENESIS;

int main(int argc, char **argv) {
	printf("filename\tsize\tmd5\tsystem\tversion\tmode\toperators\tlfo\tfb\talg\tlfo2");
	char *operators[4] = { "m1", "c1", "m2", "c2" };
	for(int i = 0; i < 4; i++) {
		printf(
			"\t%s_mult"
			"\t%s_tl"
			"\t%s_ar"
			"\t%s_dr"
			"\t%s_sl"
			"\t%s_rr"
			"\t%s_am",
			operators[i], operators[i], operators[i],
			operators[i], operators[i], operators[i],
			operators[i]
		);
		printf(
			"\t%s_ksr"
			"\t%s_dt"
			"\t%s_d2r"
			"\t%s_ssg",
			operators[i], operators[i],
			operators[i], operators[i]
		);
	}
	putchar('\n');
	for(int i = 1; i < argc; i++) {
		size_t data_len;
		uint8_t *data = load_file(argv[i], &data_len);
		if(!data) {
			fprintf(stderr, "Could not open %s\n", argv[i]);
			continue;
		}
		struct dmp_file dmp;
		if(dmp_file_load(&dmp, data, data_len, opt_system) != 0) {
			fprintf(stderr, "Could not load %s\n", argv[i]);
			continue;
		}

		// file name
		csv_quote(argv[i], 0);
		// file size
		printf("\t%lu\t", data_len);

		struct md5_ctx ctx;
		uint8_t md5buf[16];

		// file md5
		md5_init_ctx(&ctx);
		md5_process_bytes(data, data_len, &ctx);
		md5_finish_ctx(&ctx, md5buf);
		for(int j = 0; j < 16; j++) {
			printf("%02x", md5buf[j]);
		}

		// system, version, mode
		printf("\t%d\t%d\t%d", opt_system, dmp.version, dmp.mode);
		if(dmp.mode == 1) {
			// operators, lfo, fb, alg, lfo2
			printf("\t%d\t%d\t%d\t%d\t%d", dmp.num_operators, dmp.lfo, dmp.fb, dmp.alg, dmp.lfo2);
			// operators
			for(int i = 0; i < 4; i++) {
				printf(
					"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
					dmp.operators[i].mult,
					dmp.operators[i].tl,
					dmp.operators[i].ar,
					dmp.operators[i].dr,
					dmp.operators[i].sl,
					dmp.operators[i].rr,
					dmp.operators[i].am,
					dmp.operators[i].ksr,
					dmp.operators[i].dt,
					dmp.operators[i].d2r,
					dmp.operators[i].ssg
				);
			}
		}
		putchar('\n');
	}
}
