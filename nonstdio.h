/* src/akmi/nonstdio.h   26 January 1999   Alan Kennington. */
/* $Id$ */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_NONSTDIO_H
#define AKMI_NONSTDIO_H

#include "boolean.h"

#define LINEBUFSIZE 161
#define LINEBUFSIZESTR "%160s"

extern int      inch();
extern void     readln();
extern int      readname(/* FILE* f, char* buf, int bufsize */);
extern boolean  incheck();
extern void     inclear();

#endif /* AKMI_NONSTDIO_H */
