CFLAGS=-ggdb -Wall $(shell pkg-config alsa jack --cflags)
CC=gcc

PROGS=vgm2opm opmdump insdump dmpdump tfidump y12dump fmtoy_jack
all: $(PROGS)

LIBS=-lz
ifneq (,$(findstring MINGW,$(shell uname -s)))
LIBS+=-liconv -lws2_32
endif

.SECONDEXPANSION:
vgm2opm_SRCS=vgm2opm.c tools.c
opmdump_SRCS=opmdump.c tools.c opm_file.c md5.c cmdline.c
insdump_SRCS=insdump.c tools.c ins_file.c md5.c
dmpdump_SRCS=dmpdump.c tools.c dmp_file.c md5.c
tfidump_SRCS=tfidump.c tools.c tfi_file.c md5.c
y12dump_SRCS=y12dump.c tools.c y12_file.c md5.c
fmtoy_jack_SRCS=fmtoy_jack.c fmtoy.c cmdline.c tools.c \
	opm_file.c tfi_file.c ins_file.c y12_file.c dmp_file.c \
	chips/ym2151.c chips/fm.c chips/fm2612.c chips/ymdeltat.c \
	fmtoy_ym2151.c fmtoy_ym2203.c fmtoy_ym2608.c fmtoy_ym2610.c fmtoy_ym2610b.c fmtoy_ym2612.c
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
	rm -f $(PROGS) $(addsuffix .exe,$(PROGS)) *.o *.d
