/* src/akmi/message.h   26 November 1995   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_MESSAGE_H
#define AKMI_MESSAGE_H
/*---------------------------------------------------------------------------
Structs/unions/enums in this file:

union   mdmessage
struct  mdmessagebuf
enum    mdopcode
---------------------------------------------------------------------------*/

#include <stdio.h>
#include <malloc.h>
/* extern char* malloc();      /* There is no malloc.h? */

/*---------------------------------------------------------------------------
The "mdmessage" structure is intended to be used in two ways.
If the top bit is set (i.e. "op" is negative), then the message is an
instruction to delay all following messages until the clock reaches
the value (time & 0x7fff).
If the top bit of "op" is not set, then the message has a standard
opcode/arguments structure.
The units for both absolute and relative time are mS.

    top-bit     next 7 bits         lower 3 bytes

        0         opcode       args[0]  args[1] args[2]
        1         - - - - - - - - time - - - - - - - -

On a little-endian computer, "op" and "args" should probably be reversed.
---------------------------------------------------------------------------*/
typedef union MDMESSAGE { /* Message to the music daemon. */
    long time;
    struct {
        unsigned char op;   /* Should coincide with top byte of "time". */
        char args[3];       /* Lower 3 bytes of "time". */
        } msg;
    } mdmessage;

typedef struct MDMESSAGEBUF { /* Buffer for music daemon messages. */
    mdmessage* buf;     /* Beginning of buffer. */
    mdmessage* pptr;    /* "put" pointer. */
    mdmessage* gptr;    /* "get" pointer. */
    mdmessage* eptr;    /* "end" pointer. */
    } mdmessagebuf;

/* Op-code of message sent to the music daemon. */
/*
typedef enum MDOPCODE {
    opNULL, opONE, opTWO, opTHREE, opDELAY
    } mdopcode;
*/

/* Op-code of message sent to the music daemon. */
/* These are macros because the compiler won't assign an enum to a char. */
#define opNULL  0
#define opONE   1
#define opTWO   2
#define opTHREE 3
#define opDELAY 4

/* Mask for extracting times (to prevent typing errors): */
#define TIMEMASK 0x7fffffff
#define RELTIMEMASK 0x00ffffff

extern mdmessagebuf* new_mdmessagebuf();
extern void     mdmessage_show();
extern void     mdmessagebuf_dtor();
extern int      mdmessagebuf_resize();
extern int      mdmessagebuf_empty_wait();
extern int      mdmessagebuf_put_wait();
extern int      mdmessagebuf_get();
extern void     mdmessagebuf_show();

#endif /* AKMI_MESSAGE_H */
