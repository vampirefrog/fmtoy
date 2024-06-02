CFLAGS=-ggdb -Wall $(shell pkg-config alsa jack --cflags)
CC=gcc
CXX=g++

PROGS=fmtoy_jack
all: $(PROGS)

LIBS=-lz
ifneq (,$(findstring MINGW,$(shell uname -s)))
LIBS+=-liconv -lws2_32
endif

.SECONDEXPANSION:
fmtoy_jack_SRCS=fmtoy_jack.c fmtoy.c cmdline.c tools.c midi.c \
	libfmvoice/fm_voice.c libfmvoice/op3_file.c libfmvoice/opm_file.c libfmvoice/bnk_file.c libfmvoice/ins_file.c libfmvoice/sbi_file.c libfmvoice/tfi_file.c libfmvoice/y12_file.c libfmvoice/syx_dx21.c libfmvoice/syx_fb01.c libfmvoice/dmp_file.c \
	chips/ym2151.c chips/fm.c chips/fm2612.c chips/ymdeltat.c chips/fmopl.c chips/ymf262.c \
	fmtoy_ym2151.c fmtoy_ym2203.c fmtoy_ym2608.c fmtoy_ym2610.c fmtoy_ym2610b.c fmtoy_ym2612.c fmtoy_ym3812.c fmtoy_ymf262.c
fmtoy_jack_LIBS=$(shell pkg-config alsa jack --libs)

OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(foreach prog,$(PROGS),$(prog).cpp $($(prog)_SRCS))))

$(OBJS): Makefile

$(PROGS): $$(sort $$@.o $$(patsubst %.c,%.o,$$(patsubst %.cpp,%.o,$$($$@_SRCS))))
	$(CXX) $^ -o $@ $(CFLAGS) $($@_CFLAGS) $(LIBS) $($@_LIBS)

%.o: %.cpp
	$(CXX) -MMD -c $< -o $@ $(CFLAGS)
%.o: %.c
	$(CC) -MMD -c $< -o $@ $(CFLAGS) $($@_CFLAGS)

-include $(OBJS:.o=.d)

clean:
	rm -f $(PROGS) $(addsuffix .exe,$(PROGS)) *.o *.d chips/*.o chips/*.d
