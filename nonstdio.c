/* src/akmi/nonstdio.c   26 January 1999   Alan Kennington. */
/* $Id$ */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

inch
readln
readname
#ifdef AKMI_INCHECK
incheck
inclear
#endif
---------------------------------------------------------------------------*/

#include "nonstdio.h"
/* #include <osbind.h> */
#include <stdio.h>
#include <ctype.h>

/*---------------------------------------------------------------------------
inch() reads a single character from a given file, skipping white space
first. EOF is returned if the files ends, or something else goes wrong.
If end-of-line is encountered on standard input, then '\n' is returned.
---------------------------------------------------------------------------*/
/*----------------------*/
/*         inch         */
/*----------------------*/
int inch(f)
    FILE* f;
    {
    int c;

    if (!f)
        return EOF;
    while ((c = getc(f)) != EOF)
        if (isascii(c) && !isspace(c) || f == stdin && c == '\n')
            break;
    return c;
    } /* End of function inch. */

/*---------------------------------------------------------------------------
readln() reads from a given file up to the next end-of-line, positioning
the file at the first character of the next line.
---------------------------------------------------------------------------*/
/*----------------------*/
/*        readln        */
/*----------------------*/
void readln(f)
    FILE* f;
    {
    static char buf[LINEBUFSIZE];

    if (!f)
        return;
    fgets(buf, LINEBUFSIZE, f);
    } /* End of function readln. */

/*---------------------------------------------------------------------------
readname() reads from a given file up to the next non-alphabetic character,
positioning the file at that non-alphabetic character.
Return the number of characters read, or -1 if an error occurred.
If a non-negative value is returned, the name read from the file is
stored in "buf" with a null-character termination.
The returned value is the length of the name,
_not_ including the terminal null character.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       readname       */
/*----------------------*/
int readname(f, buf, bufsize)
    FILE* f;
    char* buf;
    int bufsize;
    {
    int c = 0;
    int n = 0;

    if (!f || !buf || bufsize <= 0)
        return -1;

    /* Must leave at least one character for the terminal null. */
    while (n < (bufsize - 1) && (c = getc(f)) != EOF) {
        if (!isascii(c) || !isalpha(c)) {
            /* Put back the non-alphabetic character. */
            ungetc(c, f);
            break;
            }
        buf[n] = c;
        n += 1;
        }
    /* Append a null character. */
    buf[n] = 0;

    /* Return number of characters in name, not including the null. */
    return n;
    } /* End of function readname. */

#ifdef AKMI_INCHECK

/*---------------------------------------------------------------------------
incheck() checks the console input buffer for the given character. The
function returns true if and only if the specified character is present in
the console input buffer.
---------------------------------------------------------------------------*/
/*----------------------*/
/*        incheck       */
/*----------------------*/
boolean incheck(c)
    int c;
    {
    static iorec* con_in_buffer = 0; /* Addr. of CON-in buffer descriptor. */
    iorec* p;
    int i;

    if (!con_in_buffer) {
        con_in_buffer = Iorec(1);   /* Serial input buffer 1 is for kbd. */
        if (!con_in_buffer) {
            return false;
            }
        }
    p = con_in_buffer;
/*  printf("\tBuffer is at 0x%lx\n", (long)p->ibuf);
    printf("\tBuffer size  = %d\n", p->ibufsiz);
    printf("\tHead index   = %d\n", p->ibufhd);
    printf("\tTail index   = %d\n", p->ibuftl);
    printf("\tLow mark     = %d\n", p->ibuflow);
    printf("\tHigh mark    = %d\n", p->ibufhigh); */
    for (i = p->ibufhd; i != p->ibuftl; ) {
        i += 1;
        if (i >= p->ibufsiz)
            i = 0;
        if ((i & 0x03) == 3 && p->ibuf[i] == c)
            return true;
        }
    return false;
    } /* End of function incheck. */

/*---------------------------------------------------------------------------
inclear() clears the console input buffer.
---------------------------------------------------------------------------*/
/*----------------------*/
/*        inclear       */
/*----------------------*/
void inclear()
    {
    static iorec* con_in_buffer = 0; /* Addr. of CON-in buffer descriptor. */
    iorec* p;

    if (!con_in_buffer) {
        con_in_buffer = Iorec(1);   /* Serial input buffer 1 is for kbd. */
        if (!con_in_buffer) {
            return;
            }
        }
    p = con_in_buffer;
    /* Only the head should be moved, for concurrency reasons: */
    /* This could be tricky if p->ibuftl % 4 != 0. */
    p->ibufhd = p->ibuftl & ~0x03;
    } /* End of function incheck. */
#endif
