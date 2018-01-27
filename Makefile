CFLAGS=-ggdb -Wall
CC=gcc

PROGS=vgm2opm opmdump insdump dmpdump tfidump y12dump
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
