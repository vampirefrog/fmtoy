#include <stdint.h>
#include <stdio.h>
#include "tfi_file.h"
#include "tools.h"
#include "md5.h"

int main(int argc, char **argv) {
	printf("filename\tsize\tmd5\talg\tfb");
	char *operators[4] = { "m1", "c1", "m2", "c2" };
	for(int i = 0; i < 4; i++) {
		printf(
			"\t%s_mul"
			"\t%s_dt"
			"\t%s_tl"
			"\t%s_rs"
			"\t%s_ar"
			"\t%s_dr"
			"\t%s_sr"
			"\t%s_rr"
			"\t%s_sl"
			"\t%s_ssg_eg",
			operators[i], operators[i], operators[i],
			operators[i], operators[i], operators[i],
			operators[i], operators[i], operators[i],
			operators[i]
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
		struct tfi_file tfi;
		if(tfi_file_load(&tfi, data, data_len) != 0) {
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

		// alg, fb
		printf("\t%d\t%d", tfi.alg, tfi.fb);
		for(int i = 0; i < 4; i++) {
			printf(
				"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
				tfi.operators[i].mul,
				tfi.operators[i].dt,
				tfi.operators[i].tl,
				tfi.operators[i].rs,
				tfi.operators[i].ar,
				tfi.operators[i].dr,
				tfi.operators[i].sr,
				tfi.operators[i].rr,
				tfi.operators[i].sl,
				tfi.operators[i].ssg_eg
			);
		}
		putchar('\n');
	}
}
