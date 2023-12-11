CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Werror -D_XOPEN_SOURCE=700

STRIP = 0

.PHONY: all debug release clean refresh_mtime
.SILENT: clean

all: mtime refresh_mtime libmtime.a

debug: CFLAGS += -g
debug: all

release: STRIP = 1
release: all

mtime.o: mtime.c mtime.h

mtime: CPPFLAGS = -DMTIME_TOOL
mtime: mtime.o

libmtime.a: mtime.o

refresh_mtime: mtime.o
	$(RM) $<
	$(MAKE) $<

%: %.o
	$(CC) $(filter %.o, $^) -o $@
	@if ! grep -q '$$(RM) $@$$' Makefile; then \
		printf '\t$$(RM) $@\n' >> Makefile; \
	fi
	@if [ $(STRIP) = 1 ]; then \
		strip $@; \
	fi

lib%.a: %.o
	$(AR) -rcs $@ $^
	@if ! grep -Fq '$$(RM) $@' Makefile; then \
		printf '\t$$(RM) $@\n' >> Makefile; \
	fi
	@if [ $(STRIP) = 1 ]; then \
		strip $@; \
	fi

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
	@if ! grep -Fq '$$(RM) $@' Makefile; then \
		printf '\t$$(RM) $@\n' >> Makefile; \
	fi

clean:
	sed -i '/^\t#CLEAN_START/,$${//!d}' Makefile
	#CLEAN_START
