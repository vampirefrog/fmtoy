#include <stdint.h>
#include <stdio.h>
#include "y12_file.h"
#include "tools.h"
#include "md5.h"

int main(int argc, char **argv) {
	printf("filename\tsize\tmd5\tname\tdumper\tgame\talg\tfb");
	char *operators[4] = { "m1", "c1", "m2", "c2" };
	for(int i = 0; i < 4; i++) {
		printf(
			"\t%s_mul_dt"
			"\t%s_tl"
			"\t%s_ar_rs"
			"\t%s_dr_am"
			"\t%s_sr"
			"\t%s_rr_sl"
			"\t%s_ssg",
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
		struct y12_file y12;
		if(y12_file_load(&y12, data, data_len) != 0) {
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

		csv_quote(y12.name, y12.name_len);
		putchar('\t');
		csv_quote(y12.dumper, y12.dumper_len);
		putchar('\t');
		csv_quote(y12.game, y12.game_len);

		// alg, fb
		printf("\t%d\t%d", y12.alg, y12.fb);
		for(int i = 0; i < 4; i++) {
			printf(
				"\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
				y12.operators[i].mul_dt,
				y12.operators[i].tl,
				y12.operators[i].ar_rs,
				y12.operators[i].dr_am,
				y12.operators[i].sr,
				y12.operators[i].rr_sl,
				y12.operators[i].ssg
			);
		}
		putchar('\n');
	}
}
