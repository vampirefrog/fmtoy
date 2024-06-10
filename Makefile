CC?=gcc
AR?=ar
CFLAGS?=-ggdb -Wall
ifndef EMSCRIPTEN
	CFLAGS+=$(shell pkg-config alsa jack --cflags)
endif

EMLDFLAGS?= \
	-s EXPORTED_FUNCTIONS='[\
		"_fmtoy_init",\
		"_fmtoy_load_opm_voice", \
		"_fmtoy_note_on", \
		"_fmtoy_note_off", \
		"_fmtoy_render", \
		"_fmtoy_get_buf_l", \
		"_fmtoy_get_buf_r", \
		"_fmtoy_program_change", \
		"_fmtoy_pitch_bend", \
		"_fmtoy_cc", \
		"_fmtoy_channel_name", \
		"_opm_voice", \
		"_fmtoy" \
	]' \
	-s USE_ES6_IMPORT_META=0 \
	-s ENVIRONMENT=web
LIBVGM_BUILD_DIR?=build

.PHONY: all wasm

all: libfmtoy.a fmtoy_jack
wasm: fmtoyWasm.js fmtoyAsm.js fmtoyAudioWorklet.js

LIBS=-lz -lm
ifneq (,$(findstring MINGW,$(shell uname -s)))
LIBS+=-liconv -lws2_32
endif

libfmtoy.a: fmtoy.o fmtoy_ym2151.o fmtoy_ym2203.o fmtoy_ym2608.o fmtoy_ym2610.o fmtoy_ym2610b.o fmtoy_ym2612.o fmtoy_ym3812.o fmtoy_ymf262.o
	$(AR) cr $@ $^

fmtoy_jack: fmtoy_jack.o cmdline.o tools.o libfmtoy.a libfmvoice/libfmvoice.a midilib/libmidi.a libvgm/build/bin/libvgm-emu.a
	$(CC) $^ -o $@ $(LIBS) $(shell pkg-config alsa jack --libs)

fmtowWasm.wasm fmtoyWasm.js: glue.o libfmtoy.a libfmvoice/libfmvoice.a libvgm/$(LIBVGM_BUILD_DIR)/bin/libvgm-emu.a
	$(CC) $^ -s WASM=1 -s EXPORT_ES6=1 -s MODULARIZE=1 $(EMLDFLAGS) -o $@
	sed -i '1s/^/\/* eslint-disable *\/ /' $@

fmtoyWorkletWasm.js: glue.o libfmtoy.a libfmvoice/libfmvoice.a libvgm/$(LIBVGM_BUILD_DIR)/bin/libvgm-emu.a
	$(CC) $^ -s WASM=1 EXPORT_ES6=1 -s MODULARIZE=1  -s SINGLE_FILE=1 $(EMLDFLAGS) -o $@
	sed -i '1s/^/\/* eslint-disable *\/ /' $@
	sed -i "s/typeof window == 'object' || typeof importScripts == 'function'/1/g" $@

fmtoyAsm.js: glue.o libfmtoy.a libfmvoice/libfmvoice.a libvgm/$(LIBVGM_BUILD_DIR)/bin/libvgm-emu.a
	$(CC) $^ -s WASM=0 -s EXPORT_ES6=1 -s MODULARIZE=1 $(EMLDFLAGS) -o $@
	sed -i '1s/^/\/* eslint-disable *\/ /' $@

fmtoyAudioWorklet.js: fmtoyWasm.wasm fmtoyAudioWorklet.js.template
	> $@
	echo 'const wasm = new Uint8Array([' >> $@
	cat $< | xxd -c 32 -i | sed -e 's/, /,/g' >> $@
	echo ']);' >> $@
	cat fmtoyAudioWorklet.js.template >> $@

midilib/libmidi.a:
	cd midilib && make libmidi.a
libfmvoice/libfmvoice.a:
	cd libfmvoice && make libfmvoice.a
libvgm/$(LIBVGM_BUILD_DIR)/bin/libvgm-emu.a:
	cd libvgm && cmake -B$(LIBVGM_BUILD_DIR) -DBUILD_LIBAUDIO=OFF -DBUILD_LIBPLAYER=OFF -DBUILD_PLAYER=OFF -DBUILD_VGM2WAV=OFF -DUTIL_LOADERS=OFF
	cd libvgm/$(LIBVGM_BUILD_DIR) && make vgm-emu

%.o: %.c
	$(CC) -MMD -c $< -o $@ $(CFLAGS)

-include $(OBJS:.o=.d)

clean:
	rm -f *.o *.d *.a chips/*.o chips/*.d *.js *.wasm fmtoy_jack
	cd libfmvoice && make clean
	cd midilib && make clean
	cd libvgm && rm -rf build
