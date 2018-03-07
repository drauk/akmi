/* src/akmi/message.c   24 June 1995   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

mdmessage_show

mdmessagebuf_ctor
new_mdmessagebuf
mdmessagebuf_dtor
mdmessagebuf_resize
mdmessagebuf_full
mdmessagebuf_empty
mdmessagebuf_empty_wait
mdmessagebuf_put_nowait
mdmessagebuf_put_wait
mdmessagebuf_get
mdmessagebuf_show
mdmessagebuf_play
---------------------------------------------------------------------------*/

#include "message.h"
#include "integer.h"
#include <stdio.h>

#include <sys/time.h>
#include <signal.h>

/* Terminal I/O header files etc.: */
#include <sys/termios.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <errno.h>

/* The device to be used: */
static char tty_path[] = "/dev/ttya";
static unsigned long tty_speed_mask = B38400;

static int fd_midi = 0;        /* File descriptor of MIDI interface. */

/*---------------------------------------------------------------------------
mdmessage_show() shows an mdmessage.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    mdmessage_show    */
/*----------------------*/
void mdmessage_show(p)
    mdmessage* p;
    {
    register long l;

    if (!p)
        return;
    l = p->time;
    if (l < 0) {    /* Absolute time. */
        l &= TIMEMASK;
        printf("Absolute time: %ld.\n", l);
        }
    else {          /* Opcode/args structure. */
        switch(p->msg.op) {
        case opNULL:
            printf("Null message.\n");
            break;
        case opONE:
            printf("Send 1 byte:  0x%02x.\n",
                (uint16)p->msg.args[0]);
            break;
        case opTWO:
            printf("Send 2 bytes: 0x%02x 0x%02x.\n",
                (uint16)p->msg.args[0], (uint16)p->msg.args[1]);
            break;
        case opTHREE:
            printf("Send 3 bytes: 0x%02x 0x%02x 0x%02x.\n",
                (uint16)p->msg.args[0], (uint16)p->msg.args[1],
                (uint16)p->msg.args[2]);
            break;
        case opDELAY:
            l &= RELTIMEMASK;
            printf("Delay: %ld.\n", l);
            break;
        default:
            printf("Unrecognised message: 0x%08lx.\n", l);
            break;
            }
        }
    } /* End of function mdmessage_show. */

/*---------------------------------------------------------------------------
mdmessagebuf_ctor() initialises a "mdmessagebuf".
---------------------------------------------------------------------------*/
/*----------------------*/
/*   mdmessagebuf_ctor  */
/*----------------------*/
static void mdmessagebuf_ctor(p)
    mdmessagebuf* p;
    {
    if (!p)
        return;
    p->buf = p->pptr = p->gptr = p->eptr = 0;
    } /* End of function mdmessagebuf_ctor. */

/*---------------------------------------------------------------------------
new_mdmessagebuf() returns a pointer to a new "mdmessagebuf".
---------------------------------------------------------------------------*/
/*----------------------*/
/*   new_mdmessagebuf   */
/*----------------------*/
mdmessagebuf* new_mdmessagebuf()
    {
    mdmessagebuf* p = (mdmessagebuf*)malloc(sizeof(mdmessagebuf));
    if (!p)
        return 0;
    mdmessagebuf_ctor(p);
    return p;
    } /* End of function new_mdmessagebuf. */

/*---------------------------------------------------------------------------
mdmessagebuf_dtor() destroys a "mdmessagebuf".
---------------------------------------------------------------------------*/
/*----------------------*/
/*   mdmessagebuf_dtor  */
/*----------------------*/
void mdmessagebuf_dtor(p)
    mdmessagebuf* p;
    {
    if (!p)
        return;
    if (p->buf)
        free(p->buf);
    } /* End of function mdmessagebuf_dtor. */

/*---------------------------------------------------------------------------
mdmessagebuf_resize() alters the size of a "mdmessagebuf". In the process,
all messages in the buffer are lost.
The buffer is set up so that it is empty when gptr == pptr, and full when
pptr == gptr - 1 (both modulo size).
---------------------------------------------------------------------------*/
/*----------------------*/
/*  mdmessagebuf_resize */
/*----------------------*/
int mdmessagebuf_resize(p, size)
    mdmessagebuf* p;
    int size;
    {
    if (!p || size < 0)
        return -1;
    if (p->buf) {
        free(p->buf);
        p->buf = p->pptr = p->gptr = p->eptr = 0;
        }
    if (size == 0)
        return 0;
    p->buf = (mdmessage*)malloc(size * sizeof(mdmessage));
    if (!p->buf)
        return -2;
    p->eptr = p->buf + size;
    p->pptr = p->buf;
    p->gptr = p->buf;
    return 0;
    } /* End of function mdmessagebuf_resize. */

/*---------------------------------------------------------------------------
mdmessagebuf_full() returns 1 if the buffer exists and is full; otherwise 0.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   mdmessagebuf_full  */
/*----------------------*/
static int mdmessagebuf_full(p)
    mdmessagebuf* p;
    {
    mdmessage* pp;

    if (!p || !p->buf)
        return 0;
    pp = p->pptr + 1;
    if (pp == p->eptr)
        pp = p->buf;
    return pp == p->gptr;
    } /* End of function mdmessagebuf_full. */

/*---------------------------------------------------------------------------
mdmessagebuf_empty() returns 1 if the buffer is empty or does not exist;
otherwise 0.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  mdmessagebuf_empty  */
/*----------------------*/
static int mdmessagebuf_empty(p)
    mdmessagebuf* p;
    {
    if (!p || !p->buf)
        return 1;
    return p->pptr == p->gptr;
    } /* End of function mdmessagebuf_empty. */

/*---------------------------------------------------------------------------
mdmessagebuf_empty_wait() returns 1 if the buffer is empty or does not exist;
otherwise 0. But in addition, it always waits until the buffer is empty.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  mdmessagebuf_empty_wait */
/*--------------------------*/
int mdmessagebuf_empty_wait(p)
    mdmessagebuf* p;
    {
    if (!p || !p->buf)
        return 1;
    while (p->pptr != p->gptr)
        ;
    return 1;
    } /* End of function mdmessagebuf_empty_wait. */

/*---------------------------------------------------------------------------
mdmessagebuf_put_nowait() copies a single message into a message buffer.
If the buffer is full, the function return immediately.
Return value:
-1      something went wrong
0       the buffer was full (message not put into buffer)
1       the message was put into the buffer
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  mdmessagebuf_put_nowait */
/*--------------------------*/
static int mdmessagebuf_put_nowait(p, pmsg)
    mdmessagebuf* p;
    mdmessage* pmsg;
    {
    mdmessage *pp, *pg;

    if (!p || !p->buf || !pmsg)
        return -1;
    pp = p->pptr + 1;
    if (pp == p->eptr)
        pp = p->buf;

    /* Waste CPU cycles until the buffer is non-empty: */
    pg = p->gptr; /* Hope that this is an atomic operation! */
    if (pg == pp)
        return 0;

    /* Then copy the new message, and increment the "put" pointer: */
    *p->pptr = *pmsg;
    p->pptr = pp;
    return 1;
    } /* End of function mdmessagebuf_put_nowait. */

/*---------------------------------------------------------------------------
mdmessagebuf_put_wait() copies a single message into a message buffer.
If the buffer is full, the function waits for the condition to clear.
Return value:
-1      something went wrong
1       the message was put into the buffer
---------------------------------------------------------------------------*/
/*--------------------------*/
/*   mdmessagebuf_put_wait  */
/*--------------------------*/
int mdmessagebuf_put_wait(p, pmsg)
    mdmessagebuf* p;
    mdmessage* pmsg;
    {
    mdmessage *pp, *pg;

    if (!p || !p->buf || !pmsg)
        return -1;
    pp = p->pptr + 1;
    if (pp == p->eptr)
        pp = p->buf;

    /* Waste CPU cycles until the buffer is non-empty: */
    pg = p->gptr; /* Hope that this is an atomic operation! */
    while (pg == pp)
        pg = p->gptr; /* Hope the compiler doesn't optimise this away. */

    /* Then copy the new message, and increment the "put" pointer: */
    *p->pptr = *pmsg;
    p->pptr = pp;
    return 1;
    } /* End of function mdmessagebuf_put_wait. */

/*---------------------------------------------------------------------------
mdmessagebuf_get() gets a single message from a message buffer.
If the buffer is empty, the function returns immediately.
Return values:
-1      something went wrong
0       the buffer was empty
1       a message was available, and is returned in *pmsg.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   mdmessagebuf_get   */
/*----------------------*/
int mdmessagebuf_get(p, pmsg)
    mdmessagebuf* p;
    mdmessage* pmsg;
    {
    mdmessage *pp, *pg;

    if (!p || !p->buf || !pmsg)
        return -1;
    pg = p->gptr;
    if (p->pptr == pg) /* Return if buffer empty. */
        return 0;
    *pmsg = *pg++;
    if (pg == p->eptr)
        pg = p->buf;
    p->gptr = pg;
    return 1;
    } /* End of function mdmessagebuf_get. */

/*---------------------------------------------------------------------------
mdmessagebuf_show() displays a message buffer.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   mdmessagebuf_show  */
/*----------------------*/
void mdmessagebuf_show(p)
    mdmessagebuf* p;
    {
    mdmessage* pm;
    int nlines;

    if (!p)
        return;
    printf("buf = %ld, eptr = %ld, gptr = %ld, pptr = %ld.\n",
        p->buf, p->eptr, p->gptr, p->pptr);
    nlines = 1;
    /*********p->gptr*************************/
    for (pm = p->buf; pm != p->pptr; ++pm) { /* Change this back again!!!!!*/
        if (pm == p->eptr)
            pm = p->buf;
        mdmessage_show(pm);
        ++nlines;
        if (nlines >= 10) {
            printf("--more--\n");
            if ((getchar() | 0x20) == 'q')
                break;
            nlines = 0;
            }
        }
    } /* End of function mdmessagebuf_show. */

static mdmessage* buf_play = 0;
static long buf_next_play = 0;
static long buf_play_size = 0;

/*------------------------------------------------------------------------------
This just plays the contents of an array of mdmessages.
------------------------------------------------------------------------------*/
/*----------------------*/
/* mdmessage_buffer_play*/
/*----------------------*/
static void mdmessage_buffer_play(buf, n)
    mdmessage* buf;
    int n;
    {
    if (!buf || n <= 0)
        return;
    buf_play = buf;
    buf_play_size = n;
    buf_next_play = 0;


    } /* End of function mdmessage_buffer_play. */
