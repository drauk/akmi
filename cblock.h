/* src/akmi/cblock.h   26 June 1995   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_CBLOCK_H
#define AKMI_CBLOCK_H
/*---------------------------------------------------------------------------
Structs in this file:

cblock::
cblocklist::
---------------------------------------------------------------------------*/

/* Number of characters in each cblock: */
#define CBLOCK_SIZE 100

typedef struct CBLOCK {         /* A defined cblock of events in the song. */
    struct CBLOCK* next;
    char buf[CBLOCK_SIZE];      /* Block of characters. */
    int n;                      /* Number of characters used. */
    } cblock;

typedef struct CBLOCKLIST {
    cblock *first, *last;
    long length;
    } cblocklist;

extern cblock*  new_cblock();

extern void     cblocklist_ctor();
extern void     cblocklist_clear();
extern long     cblocklist_length();
extern void     cblocklist_append();
extern void     cblocklist_putc();
extern void     cblocklist_put_bytes();
extern int      cblocklist_putmint();
extern int      cblocklist_putmint8();
extern int      cblocklist_putmint16();
extern int      cblocklist_putmint24();
extern int      cblocklist_putmint32();
extern void     cblocklist_print();

extern int      putmint();
extern int      putmint16();
extern int      putmint32();

#endif /* AKMI_CBLOCK_H */
