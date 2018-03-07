/* src/akmi/cblock.c   26 June 1995   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

new_cblock
cblocklist_ctor
cblocklist_clear
cblocklist_length
cblocklist_append
cblocklist_putc
cblocklist_put_bytes
cblocklist_putmint
cblocklist_putmint8
cblocklist_putmint16
cblocklist_putmint24
cblocklist_putmint32
cblocklist_print

putmint
putmint16
putmint32
---------------------------------------------------------------------------*/

#include "cblock.h"
#include <stdio.h>
/* #include <ctype.h> */
/* #include <string.h> */
#include <malloc.h>

/*------------------------------------------------------------------------------
new_cblock() returns an initialised new cblock.
------------------------------------------------------------------------------*/
/*----------------------*/
/*      new_cblock      */
/*----------------------*/
cblock* new_cblock()
    {
    cblock* p;

    p = (cblock*)malloc(sizeof(cblock));
    p->next = 0;
    p->n = 0;
    return p;
    } /* End of function new_cblock. */

/*------------------------------------------------------------------------------
cblocklist_ctor() constructs a cblock list
------------------------------------------------------------------------------*/
/*----------------------*/
/*    cblocklist_ctor   */
/*----------------------*/
void cblocklist_ctor(p)
    cblocklist* p;
    {
    if (!p)
        return;
    p->first = 0;
    p->last = 0;
    p->length = 0;
    } /* End of function cblocklist_ctor. */

/*------------------------------------------------------------------------------
cblocklist_clear() frees the set of all cblocks.
------------------------------------------------------------------------------*/
/*----------------------*/
/*   cblocklist_clear   */
/*----------------------*/
void cblocklist_clear(p)
    cblocklist* p;
    {
    cblock* pcb;

    if (!p)
        return;
    for (pcb = p->first; pcb; ) {
        cblock* pcb2 = pcb->next;
        free(pcb2);
        pcb = pcb2;
        }
    p->first = 0;
    p->last = 0;
    p->length = 0;
    } /* End of function cblocklist_clear. */

/*------------------------------------------------------------------------------
cblocklist_length() calculates the number of bytes in the list.
------------------------------------------------------------------------------*/
/*----------------------*/
/*   cblocklist_length  */
/*----------------------*/
long cblocklist_length(p)
    cblocklist* p;
    {
    cblock* pcb;
    long n_bytes = 0;
    if (!p)
        return 0;
    for (pcb = p->first; pcb; pcb = pcb->next)
        n_bytes += pcb->n;
    return n_bytes;
    } /* End of function cblocklist_length. */

/*---------------------------------------------------------------------------
cblocklist_append() appends a given cblock to a given list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  cblocklist_append   */
/*----------------------*/
void cblocklist_append(p, pe)
    cblocklist* p;
    cblock* pe;
    {
    if (!p || !pe)
        return;
    pe->next = 0;
    if (p->last)
        p->last->next = pe;
    else
        p->first = pe;
    p->last = pe;
    p->length += pe->n;
    } /* End of function cblocklist_append. */

/*------------------------------------------------------------------------------
cblocklist_putc() appends a character to a cblock list.
------------------------------------------------------------------------------*/
/*----------------------*/
/*    cblocklist_putc   */
/*----------------------*/
void cblocklist_putc(p, c)
    cblocklist* p;
    char c;
    {
    cblock* pcb;
    if (!p)
        return;

    pcb = p->last;
    if (!pcb || pcb->n >= CBLOCK_SIZE) {
        pcb = new_cblock();
        cblocklist_append(p, pcb);
        }
    pcb->buf[pcb->n] = c;
    pcb->n += 1;
    p->length += 1;
    } /* End of function cblocklist_putc. */

/*------------------------------------------------------------------------------
cblocklist_put_bytes() appends a byte array to a cblock list.
------------------------------------------------------------------------------*/
/*----------------------*/
/* cblocklist_put_bytes */
/*----------------------*/
void cblocklist_put_bytes(p, pc, n)
    cblocklist* p;
    char* pc;
    int n;
    {
    cblock* pcb;
    int ii = 0;

    if (!p || !pc || n <= 0)
        return;

    pcb = p->last;
    for (ii = 0; ii < n; ++ii) {
        if (!pcb || pcb->n >= CBLOCK_SIZE) {
            pcb = new_cblock();
            cblocklist_append(p, pcb);
            }
        pcb->buf[pcb->n] = *pc++;
        pcb->n += 1;
        }
    p->length += n;
    } /* End of function cblocklist_put_bytes. */

/*------------------------------------------------------------------------------
cblocklist_putmint() writes a midi variable format integer to a cblocklist.
If the cblocklist is null or the number is out of range, -1 is returned.
Otherwise the number is written, and 0 is returned.
------------------------------------------------------------------------------*/
/*----------------------*/
/*  cblocklist_putmint  */
/*----------------------*/
int cblocklist_putmint(p, x)
    cblocklist* p;
    register long x;
    {
    /* Check sanity of parameters: */
    if (!p || x < 0)
        return -1;

    if (x <= 0x7f) {
        cblocklist_putc(p, (char)x);
        }
    else if (x <= 0x3fff) {
        cblocklist_putc(p, (char)(0x80 | ((x >> 7) & 0x7f)));
        cblocklist_putc(p, (char)(x & 0x7f));
        }
    else if (x <= 0x1fffff) {
        cblocklist_putc(p, (char)(0x80 | ((x >> 14) & 0x7f)));
        cblocklist_putc(p, (char)(0x80 | ((x >> 7) & 0x7f)));
        cblocklist_putc(p, (char)(x & 0x7f));
        }
    else if (x <= 0xfffffff) {
        cblocklist_putc(p, (char)(0x80 | ((x >> 21) & 0x7f)));
        cblocklist_putc(p, (char)(0x80 | ((x >> 14) & 0x7f)));
        cblocklist_putc(p, (char)(0x80 | ((x >> 7) & 0x7f)));
        cblocklist_putc(p, (char)(x & 0x7f));
        }
    else
        return -1;
    return 0;
    } /* End of function cblocklist_putmint. */

/*------------------------------------------------------------------------------
cblocklist_putmint8() writes a midi 8-bit integer to a cblocklist.
If the cblocklist pointer is null, -1 is returned.
Otherwise the number is written, and 0 is returned.
------------------------------------------------------------------------------*/
/*----------------------*/
/*  cblocklist_putmint8 */
/*----------------------*/
int cblocklist_putmint8(p, x)
    cblocklist* p;
    register long x;
    {
    /* Check sanity of parameters: */
    if (!p)
        return -1;
    cblocklist_putc(p, (char)(x & 0xff));
    return 0;
    } /* End of function cblocklist_putmint8. */

/*------------------------------------------------------------------------------
cblocklist_putmint16() writes a midi 16-bit integer to a cblocklist.
If the cblocklist pointer is null, -1 is returned.
Otherwise the number is written, and 0 is returned.
------------------------------------------------------------------------------*/
/*----------------------*/
/* cblocklist_putmint16 */
/*----------------------*/
int cblocklist_putmint16(p, x)
    cblocklist* p;
    register long x;
    {
    /* Check sanity of parameters: */
    if (!p)
        return -1;

    cblocklist_putc(p, (char)((x >> 8) & 0xff));
    cblocklist_putc(p, (char)(x & 0xff));
    return 0;
    } /* End of function cblocklist_putmint16. */

/*------------------------------------------------------------------------------
cblocklist_putmint24() writes a midi 24-bit integer to a cblocklist.
If the cblocklist pointer is null, -1 is returned.
Otherwise the number is written, and 0 is returned.
------------------------------------------------------------------------------*/
/*----------------------*/
/* cblocklist_putmint24 */
/*----------------------*/
int cblocklist_putmint24(p, x)
    cblocklist* p;
    register long x;
    {
    /* Check sanity of parameters: */
    if (!p)
        return -1;

    cblocklist_putc(p, (char)((x >> 16) & 0xff));
    cblocklist_putc(p, (char)((x >> 8) & 0xff));
    cblocklist_putc(p, (char)(x & 0xff));
    return 0;
    } /* End of function cblocklist_putmint24. */

/*------------------------------------------------------------------------------
cblocklist_putmint32() writes a midi 32-bit integer to a cblocklist.
If the cblocklist pointer is null, -1 is returned.
Otherwise the number is written, and 0 is returned.
------------------------------------------------------------------------------*/
/*----------------------*/
/* cblocklist_putmint32 */
/*----------------------*/
int cblocklist_putmint32(p, x)
    cblocklist* p;
    register unsigned long x;
    {
    /* Check sanity of parameters: */
    if (!p)
        return -1;

    cblocklist_putc(p, (char)((x >> 24) & 0xff));
    cblocklist_putc(p, (char)((x >> 16) & 0xff));
    cblocklist_putc(p, (char)((x >> 8) & 0xff));
    cblocklist_putc(p, (char)(x & 0xff));
    return 0;
    } /* End of function cblocklist_putmint32. */

/*------------------------------------------------------------------------------
cblocklist_print() prints the entire list to a file.
------------------------------------------------------------------------------*/
/*----------------------*/
/*   cblocklist_print   */
/*----------------------*/
void cblocklist_print(p, fp)
    cblocklist* p;
    FILE* fp;
    {
    cblock* pcb;
    int i;

    if (!p || !fp)
        return;
    for (pcb = p->first; pcb; pcb = pcb->next)
        if (fwrite(pcb->buf, 1, pcb->n, fp) < pcb->n) {
            fprintf(stderr,
                "Error: fwrite() did not write sufficient chars to file.\n");
            break;
            }
    } /* End of function cblocklist_print. */

/*------------------------------------------------------------------------------
putmint() writes a midi variable format integer to a file.
If the file pointer is null, or the number is out of range, -1 is returned.
Otherwise the number is written, and 0 is returned.
------------------------------------------------------------------------------*/
/*----------------------*/
/*        putmint       */
/*----------------------*/
int putmint(fp, x)
    FILE* fp;
    register long x;
    {
    /* Check sanity of parameters: */
    if (x < 0 || !fp)
        return -1;

    if (x <= 0x7f) {
        putc(x, fp);
        }
    else if (x <= 0x3fff) {
        putc(0x80 | ((x >> 7) & 0x7f), fp);
        putc(x & 0x7f, fp);
        }
    else if (x <= 0x1fffff) {
        putc(0x80 | ((x >> 14) & 0x7f), fp);
        putc(0x80 | ((x >> 7) & 0x7f), fp);
        putc(x & 0x7f, fp);
        }
    else if (x <= 0xfffffff) {
        putc(0x80 | ((x >> 21) & 0x7f), fp);
        putc(0x80 | ((x >> 14) & 0x7f), fp);
        putc(0x80 | ((x >> 7) & 0x7f), fp);
        putc(x & 0x7f, fp);
        }
    else
        return -1;
    return 0;
    } /* End of function putmint. */

/*------------------------------------------------------------------------------
putmint16() writes a midi 16-bit integer to a file.
If the file pointer is null, -1 is returned.
Otherwise the number is written, and 0 is returned.
------------------------------------------------------------------------------*/
/*----------------------*/
/*       putmint16      */
/*----------------------*/
int putmint16(fp, x)
    FILE* fp;
    register long x;
    {
    /* Check sanity of parameters: */
    if (!fp)
        return -1;

    putc((x >> 8) & 0xff, fp);
    putc(x & 0xff, fp);
    return 0;
    } /* End of function putmint16. */

/*------------------------------------------------------------------------------
putmint32() writes a midi 32-bit integer to a file.
If the file pointer is null, -1 is returned.
Otherwise the number is written, and 0 is returned.
------------------------------------------------------------------------------*/
/*----------------------*/
/*       putmint32      */
/*----------------------*/
int putmint32(fp, x)
    FILE* fp;
    register unsigned long x;
    {
    /* Check sanity of parameters: */
    if (!fp)
        return -1;

    putc((x >> 24) & 0xff, fp);
    putc((x >> 16) & 0xff, fp);
    putc((x >> 8) & 0xff, fp);
    putc(x & 0xff, fp);
    return 0;
    } /* End of function putmint32. */
