CFLAGS=-ggdb -Wall $(shell pkg-config alsa jack --cflags)
CC=gcc
CXX=g++

all: libfmtoy.a fmtoy_jack

LIBS=-lz -lm
ifneq (,$(findstring MINGW,$(shell uname -s)))
LIBS+=-liconv -lws2_32
endif

libfmtoy.a: fmtoy.o \
	chips/ym2151.o chips/fm.o chips/fm2612.o chips/ymdeltat.o chips/fmopl.o chips/ymf262.o \
	fmtoy_ym2151.o fmtoy_ym2203.o fmtoy_ym2608.o fmtoy_ym2610.o fmtoy_ym2610b.o fmtoy_ym2612.o fmtoy_ym3812.o fmtoy_ymf262.o
	ar cr $@ $^

fmtoy_jack: fmtoy_jack.c cmdline.c tools.c libfmtoy.a libfmvoice/libfmvoice.a midilib/libmidi.a
	$(CC) $^ -o $@ $(CFLAGS) $(LIBS) $(shell pkg-config alsa jack --libs)

%.o: %.c
	$(CC) -MMD -c $< -o $@ $(CFLAGS) $($@_CFLAGS)

midilib/libmidi.a:
	cd midilib && make libmidi.a
libfmvoice/libfmvoice.a:
	cd libfmvoice && make libfmvoice.a

-include $(OBJS:.o=.d)

clean:
	rm -f *.o *.d *.a fmtoy_jack
	cd libfmvoice && make clean
	cd midilib && make clean
