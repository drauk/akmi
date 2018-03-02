# src/akmi/makefile   2018-3-2   Alan U. Kennington.
#-----------------------------------------------------------------------------
# Copyright (C) 1999, 2000, 2001, Alan U. Kennington.
# You may distribute this software under the terms of Alan U. Kennington's
# modified Artistic Licence, as specified in the accompanying LICENCE file.
#-----------------------------------------------------------------------------
# The makefile for the "akmi" program.
# This program was written in C.

all: akmi

# Application name-base.
APPL        = akmi
VERSION     = 0.1.0
CC          = gcc

CFILES = \
	akmi.c bmem.c cblock.c help.c \
	interp.c message.c nonstdio.c rat.c song.c term.c voice.c
HFILES = \
	bmem.h boolean.h cblock.h help.h integer.h \
	interp.h message.h nonstdio.h rat.h song.h term.h voice.h

CFLAGS      = -O
INCLUDES    =

# Comment this out for linux. Leave it in for SunOS 4.1.3.
# MALLOCMAP   = /usr/lib/debug/mallocmap.o

IFLAGS  = -I..
DFLAGS  = -D`arch -k` -DKERNEL
MDCFLAGS = $(DFLAGS) $(IFLAGS) -O

.c.o:
	@echo ---- Remaking \"$@\" because of:
	@echo -- $?
	$(CC) -c $(CFLAGS) $(INCLUDES) $*.c
	@echo

BOOLEAN_H   =   boolean.h

INTEGER_H   =   integer.h

NONSTDIO_H  =   nonstdio.h
nonstdio.o:     $(NONSTDIO_H)

BMEM_H      =   bmem.h
bmem.o:         $(BMEM_H)

CBLOCK_H    =   cblock.h
cblock.o:       $(CBLOCK_H)

RAT_H       =   rat.h           $(INTEGER_H) $(BOOLEAN_H)
rat.o:          $(RAT_H)        $(NONSTDIO_H)

MESSAGE_H   =   message.h
message.o:      $(MESSAGE_H)    $(INTEGER_H)

DAEMON_H    =   daemon.h        $(MESSAGE_H)
daemon.o:       $(DAEMON_H)

TERM_H      =   term.h          $(MESSAGE_H)
term.o:         $(TERM_H)

VOICE_H     =   voice.h         $(RAT_H)
voice.o:        $(VOICE_H)      $(BMEM_H)

SONG_H      =   song.h          $(VOICE_H) $(TERM_H)
song.o:         $(SONG_H)       $(CBLOCK_H)

HELP_H      =   help.h
help.o:         $(HELP_H)

INTERP_H    =   interp.h        $(TERM_H) $(SONG_H) $(BOOLEAN_H)
interp.o:       $(INTERP_H)     $(HELP_H) $(NONSTDIO_H)

akmi.o:                         $(INTERP_H) $(HELP_H) $(BMEM_H)

OBJECTS     = akmi.o interp.o help.o song.o voice.o term.o message.o rat.o \
	      cblock.o bmem.o nonstdio.o
REVOBJECTS  = nonstdio.o bmem.o cblock.o \
	      rat.o message.o term.o voice.o song.o help.o interp.o akmi.o

akmi: $(REVOBJECTS)
	$(CC) -o akmi $(OBJECTS) $(MALLOCMAP)
clean:
	rm -f akmi $(OBJECTS)
