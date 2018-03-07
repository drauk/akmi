/* src/akmi/interp.h   27 June 1995   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_INTERP_H
#define AKMI_INTERP_H
/*---------------------------------------------------------------------------
Structs in this file:
---------------------------------------------------------------------------*/

/* #include "daemon.h" */
#include "term.h"
#include "message.h"
#include "song.h"
#include "boolean.h"
/* #include <osbind.h> */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MIDIBUFSZ 256

/* sendbyte() commands: */
#define sSEND       -1
#define sSTART      -2
#define sOFF        -3
#define sPLAYBACK   -4
#define sCONTINUE   -5

extern void songinterp();
extern void interp();

#endif /* AKMI_INTERP_H */
