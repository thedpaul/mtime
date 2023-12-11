SHELL = /bin/sh

INSTALL = install

bindir = /bin
libdir = /lib

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Werror -D_XOPEN_SOURCE=700

.PHONY: all debug clean refresh_mtime
.SILENT: clean

all: mtime refresh_mtime libmtime.a

debug: CFLAGS += -g
debug: all

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

lib%.a: %.o
	$(AR) -rcs $@ $^
	@if ! grep -Fq '$$(RM) $@' Makefile; then \
		printf '\t$$(RM) $@\n' >> Makefile; \
	fi

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
	@if ! grep -Fq '$$(RM) $@' Makefile; then \
		printf '\t$$(RM) $@\n' >> Makefile; \
	fi

install:
	$(INSTALL) -D -m 755 -s mtime $(DESTDIR)$(bindir)/mtime
	$(INSTALL) -D -m 644 -s libmtime.a $(DESTDIR)$(libdir)/libmtime.a

uninstall:
	$(RM) $(DESTDIR)$(bindir)/mtime
	$(RM) $(DESTDIR)$(libdir)/libmtime.a

clean:
	sed -i '/^\t#CLEAN_START/,$${//!d}' Makefile
	#CLEAN_START
