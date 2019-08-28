#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "opm_file.h"
#include "tools.h"
#include "md5.h"
#include "cmdline.h"

char *opt_output = 0;
int opt_recursive = 0;
int opt_header = 0;

void opmdump(char *filename) {
	int len = strlen(filename);
	if(strcasecmp(filename + len - 4, ".opm")) return;


	size_t data_len;
	uint8_t *data = load_file(filename, &data_len);
	if(!data) {
		fprintf(stderr, "Could not open %s\n", filename);
		return;
	}
	struct opm_file opm;
	if(opm_file_load(&opm, data, data_len) != 0) {
		fprintf(stderr, "Could not load %s\n", filename);
		return;
	}

	for(int j = 0; j < OPM_FILE_MAX_VOICES; j++) {
		if(!opm.voices[j].used)
			continue;

		int unused = 1;
		for(int k = 0; k < 4; k++) {
			if(
				opm.voices[j].operators[k].ar != 31 ||
				opm.voices[j].operators[k].d1r != 0 ||
				opm.voices[j].operators[k].d2r != 0 ||
				opm.voices[j].operators[k].rr != 4 ||
				opm.voices[j].operators[k].d1l != 0 ||
				opm.voices[j].operators[k].tl != 0 ||
				opm.voices[j].operators[k].ks != 0 ||
				opm.voices[j].operators[k].mul != 1 ||
				opm.voices[j].operators[k].dt1 != 0 ||
				opm.voices[j].operators[k].dt2 != 0 ||
				opm.voices[j].operators[k].ame != 0
			) {
				unused = 0;
				break;
			}
		}

		if(unused &&
			opm.voices[j].lfo_lfrq == 0 &&
			opm.voices[j].lfo_amd == 0 &&
			opm.voices[j].lfo_pmd == 0 &&
			opm.voices[j].lfo_wf == 0 &&
			opm.voices[j].nfrq == 0 &&
			opm.voices[j].ch_pan == 64 &&
			opm.voices[j].ch_fl == 0 &&
			opm.voices[j].ch_con == 0 &&
			opm.voices[j].ch_ams == 0 &&
			opm.voices[j].ch_pms == 0 &&
			opm.voices[j].ch_slot == 64 &&
			opm.voices[j].ch_ne == 0
		) {
			continue;
		}

		// file name
		csv_quote(filename, 0);
		// file size
		printf("\t%lu", data_len);

		struct md5_ctx ctx;
		uint8_t md5buf[16];

		// file md5
		md5_init_ctx(&ctx);
		md5_process_bytes(data, data_len, &ctx);
		md5_finish_ctx(&ctx, md5buf);
		putchar('\t');
		for(int j = 0; j < 16; j++) {
			printf("%02x", md5buf[j]);
		}

		uint8_t buf[13 + 256 + 4 * 11];
		memset(buf, 0, sizeof(buf));
		uint8_t *p = buf;
		strcpy((char *)p, opm.voices[j].name);
		p += 256;
		*p++ = j;
		*p++ = opm.voices[j].lfo_lfrq;
		*p++ = opm.voices[j].lfo_amd;
		*p++ = opm.voices[j].lfo_pmd;
		*p++ = opm.voices[j].lfo_wf;
		*p++ = opm.voices[j].nfrq;
		*p++ = opm.voices[j].ch_pan;
		*p++ = opm.voices[j].ch_fl;
		*p++ = opm.voices[j].ch_con;
		*p++ = opm.voices[j].ch_ams;
		*p++ = opm.voices[j].ch_pms;
		*p++ = opm.voices[j].ch_slot;
		*p++ = opm.voices[j].ch_ne;
		for(int k = 0; k < 4; k++) {
			*p++ = opm.voices[j].operators[k].ar;
			*p++ = opm.voices[j].operators[k].d1r;
			*p++ = opm.voices[j].operators[k].d2r;
			*p++ = opm.voices[j].operators[k].rr;
			*p++ = opm.voices[j].operators[k].d1l;
			*p++ = opm.voices[j].operators[k].tl;
			*p++ = opm.voices[j].operators[k].ks;
			*p++ = opm.voices[j].operators[k].mul;
			*p++ = opm.voices[j].operators[k].dt1;
			*p++ = opm.voices[j].operators[k].dt2;
			*p++ = opm.voices[j].operators[k].ame;
		}

		md5_init_ctx(&ctx);
		md5_process_bytes(buf, p - buf, &ctx);
		md5_finish_ctx(&ctx, md5buf);
		putchar('\t');
		for(int j = 0; j < 16; j++) {
			printf("%02x", md5buf[j]);
		}

		md5_init_ctx(&ctx);
		md5_process_bytes(buf + 257, p - buf - 257, &ctx);
		md5_finish_ctx(&ctx, md5buf);
		putchar('\t');
		for(int j = 0; j < 16; j++) {
			printf("%02x", md5buf[j]);
		}

		printf("\t%d\t", j);
		csv_quote(opm.voices[j].name, 0);

		printf("\t%d\t%d\t%d\t%d\t%d", opm.voices[j].lfo_lfrq, opm.voices[j].lfo_amd, opm.voices[j].lfo_pmd, opm.voices[j].lfo_wf, opm.voices[j].nfrq);
		printf("\t%d\t%d\t%d\t%d\t%d\t%d\t%d", opm.voices[j].ch_pan, opm.voices[j].ch_fl, opm.voices[j].ch_con, opm.voices[j].ch_ams, opm.voices[j].ch_pms, opm.voices[j].ch_slot, opm.voices[j].ch_ne);

		for(int k = 0; k < 4; k++) {
			printf(
				"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
				opm.voices[j].operators[k].ar,
				opm.voices[j].operators[k].d1r,
				opm.voices[j].operators[k].d2r,
				opm.voices[j].operators[k].rr,
				opm.voices[j].operators[k].d1l,
				opm.voices[j].operators[k].tl,
				opm.voices[j].operators[k].ks,
				opm.voices[j].operators[k].mul,
				opm.voices[j].operators[k].dt1,
				opm.voices[j].operators[k].dt2,
				opm.voices[j].operators[k].ame
			);
		}

		putchar('\n');
	}
}

static void opmdump_dir(const char *name, int recurse) {
	DIR *dir;
	struct dirent *entry;

	if(!(dir = opendir(name)))
		return;

	while((entry = readdir(dir)) != NULL) {
		char path[1024];
		if (entry->d_type == DT_DIR) {
			if(!recurse)
				continue;
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
			opmdump_dir(path, recurse);
		} else {
			snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
			opmdump(path);
		}
	}
	closedir(dir);
}

int main(int argc, char **argv) {

	int optind = cmdline_parse_args(argc, argv, (struct cmdline_option[]){
		{
			'o', "output",
			"Output file",
			"file",
			TYPE_REQUIRED,
			TYPE_STRING, &opt_output
		},
		{
			'r', "recursive",
			"Recurse directories",
			0,
			TYPE_SWITCH,
			TYPE_NONE, &opt_recursive
		},
		{
			'H', "header",
			"Print header",
			0,
			TYPE_SWITCH,
			TYPE_NONE, &opt_header
		},
		CMDLINE_ARG_TERMINATOR
	}, 1, 0, "<file.opm>");

	if(optind < 0) exit(-optind);

	if(opt_header) {
		printf(
			"filename\tsize\tfile_md5\tvoice_md5\tvoice_data_md5\tnum\tname"
			"\tlfo_lfrq\tlfo_amd\tlfo_pmd\tlfo_wf\tlfo_nfrq"
			"\tch_pan\tch_fl\tch_con\tch_ams\tch_pms\tch_slot\tch_ne"
		);
		char *operators[4] = { "m1", "c1", "m2", "c2" };
		for(int i = 0; i < 4; i++) {
			printf(
				"\t%s_ar"
				"\t%s_d1r"
				"\t%s_d2r"
				"\t%s_rr"
				"\t%s_d1l"
				"\t%s_tl"
				"\t%s_ks"
				"\t%s_mul"
				"\t%s_dt1"
				"\t%s_dt2"
				"\t%s_ame",
				operators[i], operators[i], operators[i],
				operators[i], operators[i], operators[i],
				operators[i], operators[i], operators[i],
				operators[i], operators[i]
			);
		}
		putchar('\n');
	}

	for(int i = optind; i < argc; i++) {
		struct stat st;
		stat(argv[i], &st);
		if(S_ISDIR(st.st_mode))
			opmdump_dir(argv[i], opt_recursive);
		else
			opmdump(argv[i]);
	}
}
