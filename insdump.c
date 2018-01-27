#include <stdint.h>
#include <stdio.h>
#include "ins_file.h"
#include "tools.h"
#include "md5.h"

int main(int argc, char **argv) {
	printf("filename\tsize\tmd5\tdata_md5\tname\tversion\tfb_alg");
	char *operators[4] = { "m1", "c1", "m2", "c2" };
	for(int i = 0; i < 4; i++) {
		printf(
			"\t%s_mul_dt"
			"\t%s_tl"
			"\t%s_rs_ar"
			"\t%s_dr"
			"\t%s_sr"
			"\t%s_sl_rr",
			operators[i], operators[i], operators[i],
			operators[i], operators[i], operators[i]
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
		struct ins_file ins;
		if(ins_file_load(&ins, data, data_len) != 0) {
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
		putchar('\t');

		// data md5 (after name)
		md5_init_ctx(&ctx);
		md5_process_bytes(data + ins.data_offset, data_len - ins.data_offset, &ctx);
		md5_finish_ctx(&ctx, md5buf);
		for(int j = 0; j < 16; j++) {
			printf("%02x", md5buf[j]);
		}
		putchar('\t');

		// name
		csv_quote(ins.name, 0);
		// version, fb_alg
		printf("\t%d\t%d", ins.version, ins.fb_alg);
		// operators
		for(int i = 0; i < 4; i++) {
			printf(
				"\t%d\t%d\t%d\t%d\t%d\t%d",
				ins.operators[i].mul_dt,
				ins.operators[i].tl,
				ins.operators[i].rs_ar,
				ins.operators[i].dr,
				ins.operators[i].sr,
				ins.operators[i].sl_rr
			);
		}
		putchar('\n');
	}
}
