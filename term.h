/* src/akmi/term.h   26 November 1995   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_TERM_H
#define AKMI_TERM_H
/*---------------------------------------------------------------------------
Structs/unions/enums in this file:

---------------------------------------------------------------------------*/

#include "message.h"
/* #include <osbind.h> */

extern int Midiws();

extern void mdmessage_buffer_play();

extern int  mdaemon_init();
extern int  mdaemon_term();
extern int  mdaemon_term_wait();
extern long mdaemon_time();
extern void mdaemon_clock_set();
extern void mdaemon_clock_start();
extern void mdaemon_clock_stop();
extern long mdaemon_maxdelay();
extern void mdaemon_sendbytes_wait();
extern void mdaemon_senddelay_wait();
extern void mdaemon_show();
extern void mdaemon_read_midi_buf();

/* At 4 bytes per message, this is 2MB of buffer: */
#define mdmessage_buffer_size 500000
extern mdmessage mdmessage_buffer[mdmessage_buffer_size];

#endif /* AKMI_TERM_H */
