/* src/akmi/term.c   2018-3-3   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

Midiws
xhandler
mdmessage_buffer_play
bool
baud_rate_print
termios_print
modem_lines_print

terminate
mdaemon_init
// mdaemon_init
mdaemon_term
mdaemon_term_wait
mdaemon_time
mdaemon_clock_set
mdaemon
mdaemon_handler
mdaemon_clock_start
mdaemon_clock_stop
mdaemon_maxdelay
mdaemon_sendbytes_wait
// mdaemon_sendbytes_wait
mdaemon_senddelay_wait
mdaemon_show
mdaemon_read_midi_buf
// A_handler
---------------------------------------------------------------------------*/

#include "term.h"
#ifndef AKMI_BOOLEAN_H
#include "boolean.h"
#endif

/* System header files. */
#ifndef AKSL_X_STDIO_H
#define AKSL_X_STDIO_H
#include <stdio.h>
#endif
#ifndef AKSL_X_SYS_TIME_H
#define AKSL_X_SYS_TIME_H
#include <sys/time.h>
#endif
#ifndef AKSL_X_SIGNAL_H
#define AKSL_X_SIGNAL_H
#include <signal.h>
#endif

/* Terminal I/O header files etc.: */
#ifndef AKSL_X_SYS_TERMIOS_H
#define AKSL_X_SYS_TERMIOS_H
#include <sys/termios.h>
#endif
#ifndef AKSL_X_SYS_UNISTD_H
#define AKSL_X_SYS_UNISTD_H
#include <sys/unistd.h>
#endif
#ifndef AKSL_X_FCNTL_H
#define AKSL_X_FCNTL_H
#include <fcntl.h>
#endif
#ifndef AKSL_X_ERRNO_H
#define AKSL_X_ERRNO_H
#include <errno.h>
#endif

#ifdef linux
#ifndef AKSL_X_SYS_IOCTL_H
#define AKSL_X_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifndef AKSL_X_SYS_TYPES_H
#define AKSL_X_SYS_TYPES_H
#include <sys/types.h>
#endif
#endif

/* The device to be used: */
static char tty_path[] = "/dev/ttya";
static unsigned long tty_speed_mask = B38400;

/* extern void A_handler(); /* "extern" because of strange asm implementation.*/

static int fd_midi = 0;             /* File descriptor of MIDI interface. */

#define CON_PORT ((int)2)           /* For Bconstat() etc. */
#define MIDI_PORT ((int)3)          /* For Bconstat() etc. */
#define DEFTMSGBUFSIZE 1024
static mdmessagebuf* msgbuf = 0;    /* Buffer for music daemon messages. */

/* Semaphore to prevent daemon clashes: (0 for sleeping, 0xff for busy.) */
static unsigned char mdaemon_busy = 0; /* Accessed via "tas" operation. */

static unsigned long mclock = 0; /* Real-time clock for sequencing. (mS) */
static long mclock_usec = 0;     /* Correction to real-time clock. (uS) */
static int clock_on = 0;         /* True if clock is running. */
static long maxdelay = 0;  /* Maximum delay to be tolerated in sending. (mS) */

static mdmessage nextmsg;       /* Next action to be taken. */
static long nextmsgtime = 0;    /* Time of next action to be taken. (mS) */

static int mdaemon_exists = 0; /* True if timer interrupts are activated. */
static void (*oldterminate)() = 0;  /* "terminate" vector to be saved. */
/* static iorec* midi_in_buffer = 0; /* Address of MIDI-in buffer descriptor. */

/* Totally ad hoc array, just to get the thing going: */
mdmessage mdmessage_buffer[mdmessage_buffer_size];

/* The buffer is _not_ circular. That's why it's just experimental. */
static long mdp = 0;        /* End of contents of the buffer. (= pptr) */
static long mdp_play = 0;   /* Next message to be played. (= gptr) */

/*----------------------*/
/*        Midiws        */
/*----------------------*/
int Midiws(n, args)
    int n;
    unsigned char* args;
    {
    int nbytes = 0;

    if (n <= 0 || !args)
        return 0;
    if (!mdaemon_exists)
        mdaemon_init();

    nbytes = write(fd_midi, (char*)args, n);
    return nbytes;
    } /* End of function Midiws. */

/*------------------------------------------------------------------------------
This is an ad hoc handler.
------------------------------------------------------------------------------*/
/*----------------------*/
/*        xhandler      */
/*----------------------*/
static void xhandler(sig, code, scp, addr)
    int sig;                    /* The signal number. */
    int code;                   /* Some signals provide this ?! */
    struct sigcontext* scp;     /* For restoring context from before signal. */
    char* addr;                 /* Additional information?! */
    {
    mdmessage msg;
    long next_delay = 0;
    struct itimerval value, ovalue;
    long q = 0;
    long r = 0;

    /* If the message buffer is empty, return (i.e. stop the interrupts): */
    if (mdp_play >= mdp)
        return;
    msg.time = mdmessage_buffer[mdp_play++].time;

    next_delay = 0;
    for (;;) {
        if (msg.time & 0x8000) { /* Case of an absolute time instruction. */
/*
            printf("msg.time = %#lx.\n", (long)msg.time);
*/
            nextmsgtime = msg.time & TIMEMASK;
            if (nextmsgtime > mclock)
                next_delay = nextmsgtime - mclock;
            }
        else { /* Case of an opcode/args instruction. */
            switch(msg.msg.op) {
            case opNULL:    /* Do-nothing opcode. */
                break;
            case opONE:     /* Send only args[0]. */
                Midiws(1, msg.msg.args);
                break;
            case opTWO:     /* Send only args[0] and args[1]. */
                Midiws(2, msg.msg.args);
                break;
            case opTHREE:   /* Send args[0], args[1] and args[2]. */
                Midiws(3, msg.msg.args);
                break;
            case opDELAY:   /* Make a relative change to "nextmsgtime". */
/*
                printf("msg.time = %#lx.\n", (long)msg.time);
*/
                nextmsgtime += msg.time & RELTIMEMASK;
                if (nextmsgtime <= mclock)
                    break;
                next_delay = nextmsgtime - mclock;
                break;
            default:        /* Ignore all other errors. */
                break;
                } /* End of switch. */
            }

        /* Fetch another instruction, and execute it if it is time: */
        if (next_delay > 0) {
            /* Increment mclock_usec by number of uS in next_delay: */
            mclock_usec += next_delay * 1000;

            /* Calculate r = ceiling(mclock_usec/16666) * 16666: */
            r = (mclock_usec/16666) * 16666;
            if (r < mclock_usec)
                r += 16666;

            /* Add floor(r/1000) to mclock:*/
/*
            mclock += r / 1000;
            mclock_usec -= r % 1000;
*/
            mclock += mclock_usec / 1000;
            mclock_usec %= 1000;

            /* Set timer to call back in r uS: */
            r = next_delay * 1000;
            q = r/1000000;
            r -= q * 1000000;
/*
            printf("q = %ld; r = %ld;", (long)q, (long)r);
            printf(" nextmsgtime = %ld\n", (long)nextmsgtime);
*/
            value.it_value.tv_sec =  q; /* First time-out interval. */
            value.it_value.tv_usec = r; /* First time-out interval. */
            value.it_interval.tv_sec =  0; /* Subsequent time-out intervals. */
            value.it_interval.tv_usec = 0; /* Subsequent time-out intervals. */

            setitimer((int)ITIMER_REAL, &value, &ovalue);
            return;
            }
        if (mdp_play >= mdp)
            return;
        msg.time = mdmessage_buffer[mdp_play++].time;
        }
    } /* End of function xhandler. */

/*------------------------------------------------------------------------------
This just plays the contents of an array of mdmessages.
------------------------------------------------------------------------------*/
/*----------------------*/
/* mdmessage_buffer_play*/
/*----------------------*/
void mdmessage_buffer_play(buf)
    mdmessage* buf;
    {
    struct sigvec vec, ovec;
    struct itimerval value, ovalue;
    int ret = 0;

    if (!buf)
        return;
/*    buf_play = buf;
    buf_play_size = n;
    buf_next_play = 0; */

    mclock_usec = 0;
    clock_on = 1;

    /* Set the handler for the SIGALRM signal from setitimer(): */
    /* (The old signal vector is returned in "ovec".) */
    vec.sv_handler = xhandler;
    vec.sv_mask = 0;                /* Don't mask off other signals. */
    vec.sv_flags = 0;               /* Specify no extra options. */
    ret = sigvec((int)SIGALRM, &vec, &ovec);

    /* Set the first timeout and subsequent timeouts for setitimer: */
    value.it_value.tv_sec =         0; /* First time-out interval. */
    value.it_value.tv_usec =    49998; /* First time-out interval. */
    value.it_interval.tv_sec =      0; /* Subsequent time-out intervals. */
    value.it_interval.tv_usec =     0; /* Subsequent time-out intervals. */
    setitimer((int)ITIMER_REAL, &value, &ovalue);
    } /* End of function mdmessage_buffer_play. */

/*----------------------*/
/*         bool         */
/*----------------------*/
static char* bool(i)
    long i;
    {
    static char* t = "true";
    static char* f = "false";
    return (i == 0) ? f : t;
    } /* End of function bool. */

/*----------------------*/
/*    baud_rate_print   */
/*----------------------*/
static void baud_rate_print(cbaud)
    unsigned long cbaud;
    {
    long b = 0;
    switch(cbaud) {
    case B0: b = 0; break;
    case B50: b = 50; break;
    case B75: b = 75; break;
    case B110: b = 110; break;
    case B134: b = 134; break;
    case B150: b = 150; break;
    case B200: b = 200; break;
    case B300: b = 300; break;
    case B600: b = 600; break;
    case B1200: b = 1200; break;
    case B1800: b = 1800; break;
    case B2400: b = 2400; break;
    case B4800: b = 4800; break;
    case B9600: b = 9600; break;
    case B19200: b = 19200; break;
    case B38400: b = 38400; break;
    default:
        b = -1;
        printf("Unrecognized baud rate.\n");
        break;
        } /* End of switch(cbaud). */
    if (b >= 0)
        printf("    Baud rate = %ld.\n", b);
    } /* End of function baud_rate_print. */

/*----------------------*/
/*     termios_print    */
/*----------------------*/
static void termios_print(tio)
    struct termios* tio;
    {
    int cs = 0;
    long csize = 0;
    unsigned long cbaud = 0;
#ifndef linux
    unsigned long cibaud = 0;
#endif
    if (!tio)
        return;
    printf("c_iflag = %#lx.\n", (long)tio->c_iflag);
    printf("    IGNBRK   = %s.\n", bool(IGNBRK  & tio->c_iflag));
    printf("    BRKINT   = %s.\n", bool(BRKINT  & tio->c_iflag));
    printf("    IGNPAR   = %s.\n", bool(IGNPAR  & tio->c_iflag));
    printf("    PARMRK   = %s.\n", bool(PARMRK  & tio->c_iflag));
    printf("    INPCK    = %s.\n", bool(INPCK   & tio->c_iflag));
    printf("    ISTRIP   = %s.\n", bool(ISTRIP  & tio->c_iflag));
    printf("    INLCR    = %s.\n", bool(INLCR   & tio->c_iflag));
    printf("    IGNCR    = %s.\n", bool(IGNCR   & tio->c_iflag));
    printf("    ICRNL    = %s.\n", bool(ICRNL   & tio->c_iflag));
    printf("    IUCLC    = %s.\n", bool(IUCLC   & tio->c_iflag));
    printf("    IXON     = %s.\n", bool(IXON    & tio->c_iflag));
    printf("    IXANY    = %s.\n", bool(IXANY   & tio->c_iflag));
    printf("    IXOFF    = %s.\n", bool(IXOFF   & tio->c_iflag));
    printf("    IMAXBEL  = %s.\n", bool(IMAXBEL & tio->c_iflag));

    printf("c_oflag = %#lx.\n", (long)tio->c_oflag);
    printf("    OPOST    = %s.\n", bool(OPOST   & tio->c_oflag));
    printf("    OLCUC    = %s.\n", bool(OLCUC   & tio->c_oflag));
    printf("    ONLCR    = %s.\n", bool(ONLCR   & tio->c_oflag));
    printf("    OCRNL    = %s.\n", bool(OCRNL   & tio->c_oflag));
    printf("    ONOCR    = %s.\n", bool(ONOCR   & tio->c_oflag));
    printf("    ONLRET   = %s.\n", bool(ONLRET  & tio->c_oflag));
    printf("    OFILL    = %s.\n", bool(OFILL   & tio->c_oflag));
    printf("    OFDEL    = %s.\n", bool(OFDEL   & tio->c_oflag));
    printf("    NLDLY    = %s.\n", bool(NLDLY   & tio->c_oflag));
    printf("    CRDLY    = %#lx.\n", (long)((CRDLY & tio->c_oflag) >> 9));
    printf("    TABDLY   = %#lx.\n", (long)((TABDLY & tio->c_oflag) >> 11));
    printf("    BSDLY    = %s.\n", bool(BSDLY   & tio->c_oflag));
    printf("    VTDLY    = %s.\n", bool(VTDLY   & tio->c_oflag));
    printf("    FFDLY    = %s.\n", bool(FFDLY   & tio->c_oflag));

    printf("c_cflag = %#lx.\n", (long)(tio->c_cflag));
    cbaud = CBAUD & tio->c_cflag;
    printf("    CBAUD    = %#lx.\n", (long)cbaud);
    baud_rate_print(cbaud);

    csize = CSIZE & tio->c_cflag;
    printf("    CSIZE    = %#lx.\n", (long)(csize >> 4));
    switch(csize) {
    case CS5: cs = 5; break;
    case CS6: cs = 6; break;
    case CS7: cs = 7; break;
    case CS8: cs = 8; break;
        } /* End of switch(csize). */
    printf("Character size = %ld bits.\n", (long)cs);
    printf("    CSTOPB   = %s.\n", bool(CSTOPB   & tio->c_cflag));
    printf("    CREAD    = %s.\n", bool(CREAD    & tio->c_cflag));
    printf("    PARENB   = %s.\n", bool(PARENB   & tio->c_cflag));
    printf("    PARODD   = %s.\n", bool(PARODD   & tio->c_cflag));
    printf("    HUPCL    = %s.\n", bool(HUPCL    & tio->c_cflag));
    printf("    CLOCAL   = %s.\n", bool(CLOCAL   & tio->c_cflag));
#ifndef linux
    cibaud = CIBAUD & tio->c_cflag;
    printf("    CIBAUD   = %#lx.\n", (long)(cibaud >> IBSHIFT));
    printf("Input baud rate:\n");
    baud_rate_print(cibaud >> IBSHIFT);
#endif
    printf("    CRTSCTS  = %s.\n", bool(CRTSCTS  & tio->c_cflag));

    printf("c_lflag = %#lx.\n", (long)(tio->c_lflag));
    printf("    ISIG     = %s.\n", bool(ISIG     & tio->c_lflag));
    printf("    ICANON   = %s.\n", bool(ICANON   & tio->c_lflag));
    printf("    XCASE    = %s.\n", bool(XCASE    & tio->c_lflag));
    printf("    ECHO     = %s.\n", bool(ECHO     & tio->c_lflag));
    printf("    ECHOE    = %s.\n", bool(ECHOE    & tio->c_lflag));
    printf("    ECHOK    = %s.\n", bool(ECHOK    & tio->c_lflag));
    printf("    ECHONL   = %s.\n", bool(ECHONL   & tio->c_lflag));
    printf("    NOFLSH   = %s.\n", bool(NOFLSH   & tio->c_lflag));
    printf("    TOSTOP   = %s.\n", bool(TOSTOP   & tio->c_lflag));
    printf("    ECHOCTL  = %s.\n", bool(ECHOCTL  & tio->c_lflag));
    printf("    ECHOPRT  = %s.\n", bool(ECHOPRT  & tio->c_lflag));
    printf("    ECHOKE   = %s.\n", bool(ECHOKE   & tio->c_lflag));
    printf("    FLUSHO   = %s.\n", bool(FLUSHO   & tio->c_lflag));
    printf("    PENDIN   = %s.\n", bool(PENDIN   & tio->c_lflag));
    printf("    IEXTEN   = %s.\n", bool(IEXTEN   & tio->c_lflag));

    printf("c_line  = %#lx.\n", (long)(tio->c_line));
    printf("    VINTR    = %#lx.\n", (long)(tio->c_cc[0]));
    printf("    VQUIT    = %#lx.\n", (long)(tio->c_cc[1]));
    printf("    VERASE   = %#lx.\n", (long)(tio->c_cc[2]));
    printf("    VKILL    = %#lx.\n", (long)(tio->c_cc[3]));
    printf("    VEOF     = %#lx.\n", (long)(tio->c_cc[4]));
    printf("    VEOL     = %#lx.\n", (long)(tio->c_cc[5]));
    printf("    VEOL2    = %#lx.\n", (long)(tio->c_cc[6]));
    printf("    VSWTCH   = %#lx.\n", (long)(tio->c_cc[7]));
    printf("    VSTART   = %#lx.\n", (long)(tio->c_cc[8]));
    printf("    VSTOP    = %#lx.\n", (long)(tio->c_cc[9]));
    printf("    VSUSP    = %#lx.\n", (long)(tio->c_cc[10]));
    printf("             = %#lx.\n", (long)(tio->c_cc[11]));
    printf("    VREPRINT = %#lx.\n", (long)(tio->c_cc[12]));
    printf("    VDISCARD = %#lx.\n", (long)(tio->c_cc[13]));
    printf("    VWERASE  = %#lx.\n", (long)(tio->c_cc[14]));
    printf("    VLNEXT   = %#lx.\n", (long)(tio->c_cc[15]));
    printf("             = %#lx.\n", (long)(tio->c_cc[16]));
    } /* End of function termios_print. */

/*----------------------*/
/*   modem_lines_print  */
/*----------------------*/
static void modem_lines_print(modem_lines)
    int modem_lines;
    {
    printf("Line enable         = %s.\n", bool(TIOCM_LE  & modem_lines));
    printf("Data terminal ready = %s.\n", bool(TIOCM_DTR & modem_lines));
    printf("Request to send     = %s.\n", bool(TIOCM_RTS & modem_lines));
    printf("Secondary transmit  = %s.\n", bool(TIOCM_ST  & modem_lines));
    printf("Secondary receive   = %s.\n", bool(TIOCM_SR  & modem_lines));
    printf("Clear to send       = %s.\n", bool(TIOCM_CTS & modem_lines));
    printf("Carrier detect      = %s.\n", bool(TIOCM_CAR & modem_lines));
    printf("Ring                = %s.\n", bool(TIOCM_RNG & modem_lines));
    printf("Data set ready      = %s.\n", bool(TIOCM_DSR & modem_lines));
    } /* End of function modem_lines_print. */

/*---------------------------------------------------------------------------
terminate() is a function to stop the A-timer calling mdaemon, in case the
user forgets, or there is an abnormal termination of the program.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       terminate      */
/*----------------------*/
static void terminate()
    {
    mdaemon_term(); /* Forward reference, but also an exported function. */
    } /* End of function terminate. */

/*----------------------*/
/*     mdaemon_init     */
/*----------------------*/
int mdaemon_init()
    {
    struct termios tio;
    int ioctl_error = 0;
    int modem_lines_mask = 0;
    int i = 0;
    int cx = 0;
    int nbytes = 0;

    static unsigned char test_msg[] = {
        0x90, 0x3c, 0x40, 0x90, 0x3e, 0x40, 0x90, 0x40, 0x40,
        0x80, 0x3c, 0x40, 0x80, 0x3e, 0x40, 0x80, 0x40, 0x40,
        0x90, 0x3c, 0x40, 0x90, 0x3e, 0x40, 0x90, 0x40, 0x40,
        0x80, 0x3c, 0x40, 0x80, 0x3e, 0x40, 0x80, 0x40, 0x40,
        0x90, 0x3c, 0x40, 0x90, 0x3e, 0x40, 0x90, 0x40, 0x40,
        0x80, 0x3c, 0x40, 0x80, 0x3e, 0x40, 0x80, 0x40, 0x40,
        0x90, 0x3c, 0x40, 0x90, 0x3e, 0x40, 0x90, 0x40, 0x40,
        0x80, 0x3c, 0x40, 0x80, 0x3e, 0x40, 0x80, 0x40, 0x40
        };

    if (mdaemon_exists)
        return -1;

    /* Open the MIDI device: */
    fd_midi = open(tty_path, (int)(O_RDWR|O_NDELAY));
/*
    printf("Got fd_midi = %ld.\n", (long)fd_midi);
*/
    if (fd_midi < 0) {
        printf("Got errno = %ld.\n", (long)errno);
        perror("mdaemon_init: open(tty_path, (int)(O_RDWR|O_NDELAY))");
        return -1;
        }

    /* Set the software DCD mode to "on": */
    i = 1;
    ioctl_error = ioctl(fd_midi, (int)TIOCSSOFTCAR, (caddr_t)&i);
/*
    printf("Got return value %ld from ioctl(fd_midi, TIOCSSOFTCAR).\n",
           (long)ioctl_error);
*/
    if (ioctl_error < 0) {
        printf("Got error number err = %ld from ioctl().\n", (long)errno);
        perror("mdaemon_init: ioctl TIOCSSOFTCAR");
        return -1;
        }

    /* Read the status of the MIDI device: */
    ioctl_error = ioctl(fd_midi, (int)TCGETS, (caddr_t)&tio);
    if (ioctl_error < 0) {
        printf("Got error number err = %ld from ioctl().\n", (long)errno);
        perror("mdaemon_init: ioctl TCGETS");
        return -1;
        }
/*
    printf("Found following contents in termios structure.\n");
    termios_print(&tio);
*/

    /* Set the modem line DTR low: */
    modem_lines_mask = TIOCM_DTR;
    ioctl_error = ioctl(fd_midi, (int)TIOCMBIC, (caddr_t)&modem_lines_mask);
    if (ioctl_error < 0) {
        printf("Got error number err = %ld from ioctl().\n", (long)errno);
        perror("mdaemon_init: ioctl TIOCMBIC");
        return -1;
        }

    /* Turn on certain iflags (because it's in kermit): */
/*    tio.c_iflag |= (BRKINT|IGNPAR|IXON|IXOFF); */
    tio.c_iflag |= (IGNBRK|BRKINT|IGNPAR);

    /* Turn off iflags (because it's in kermit): */
/*    tio.c_iflag &= ~(IGNBRK|INLCR|IGNCR|ICRNL|IUCLC|ISTRIP); */
    tio.c_iflag &= ~(INLCR|IGNCR|ICRNL|IUCLC|IXON|IXOFF|ISTRIP);

    /* Turn off oflags (because it's in kermit): */
    tio.c_oflag &= ~(ONLCR|OCRNL|ONLRET|OPOST);

    /* Modify output baud rate: */
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= tty_speed_mask;
#ifndef linux
    /* Modify input baud rate: */
    tio.c_cflag &= ~CIBAUD;
    tio.c_cflag |= (tty_speed_mask << IBSHIFT) & CIBAUD;
#endif
    /* Change from 7-bit to 8-bit: */
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;

    /* Turn on RTS-CTS: */
    tio.c_cflag |= (CLOCAL|HUPCL|CRTSCTS);
/*    tio.c_cflag |= (CLOCAL|HUPCL); */

    /* Turn off parity enable: */
    tio.c_cflag &= ~PARENB;
/*    tio.c_cflag &= ~(PARENB|CRTSCTS); */

    /* Turn off echo and canonical mode: */
    tio.c_lflag &= ~(ISIG|ICANON|IEXTEN|ECHO);
/*    tio.c_lflag &= ~(ISIG|ECHO); */

    tio.c_cc[VMIN]  = 1;    /* Same as VEOF. */
    tio.c_cc[VTIME] = 0;    /* Same as VEOL. */

    ioctl_error = ioctl(fd_midi, (int)TCSETS, (caddr_t)&tio);
    if (ioctl_error < 0) {
        printf("Got error number err = %ld from ioctl().\n", (long)errno);
        perror("mdaemon_init: ioctl TCSETS");
        return -1;
        }
/*
    for (cx = 0; cx < sizeof(test_msg); cx += 3) {
        nbytes = write(fd_midi, (char*)&test_msg[cx], (int)3);
        if (nbytes < 0)
            break;
        usleep(500000);
        }
*/
    clock_on = 0;
    mclock = 0;
    nextmsg.time = 0; /* Clear the waiting message. */
    nextmsgtime = 0;
    mdp_play = 0;
    mdaemon_busy = 0;

    mdaemon_exists = 1;
    return 0;
    } /* End of function mdaemon_init. */

/*---------------------------------------------------------------------------
mdaemon_init() is called to initialise a daemon. The daemon must be
terminated before the program exits.
Return values:
0       success
-1      a daemon already exists
-2      couldn't create a new message buffer
---------------------------------------------------------------------------*/
/*----------------------*/
/*     mdaemon_init     */
/*----------------------*/
/*
int mdaemon_init()
    {
    if (mdaemon_exists)
        return -1;
    if (msgbuf)
        free(msgbuf);
    msgbuf = new_mdmessagebuf();
    if (!msgbuf)
        return -2;
    mdmessagebuf_resize(msgbuf, DEFTMSGBUFSIZE);

    // Replace the system "terminate" vector, in case of hasty exit:
    oldterminate = (void (*)())Setexc(0x102, terminate);

    mclock = 0;
    clock_on = 0;
    nextmsg.time = 0; // Clear the waiting message.
    nextmsgtime = 0;
    mdaemon_busy = 0;

    // Timer A, prescale = 64, data = 192, interrupt routine A_handler:
//    Xbtimer(0, 5, 192, A_handler);
    mdaemon_exists = 1;
    return 0;
    } // End of function mdaemon_init.
*/

/*---------------------------------------------------------------------------
mdaemon_term() is called to terminate a daemon. The daemon must be
terminated before the main program exits.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     mdaemon_term     */
/*----------------------*/
int mdaemon_term()
    {
    if (!mdaemon_exists)
        return -1;
/*    Xbtimer(0, 0, 0, (long)0); /* Clear the A-timer interrupt routine. */
    mdaemon_exists = 0;
/*    Setexc(0x102, oldterminate); */
/*    mdmessagebuf_dtor(msgbuf); */
    msgbuf = 0;
    return 0;
    } /* End of function mdaemon_term. */

/*---------------------------------------------------------------------------
mdaemon_term_wait() is called to terminate a daemon. The daemon must be
terminated before the main program exits.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   mdaemon_term_wait  */
/*----------------------*/
int mdaemon_term_wait()
    {
    if (!mdaemon_exists)
        return -1;
    mdmessagebuf_empty_wait(msgbuf); /* Wait for all messages to be sent. */
/*    Xbtimer(0, 0, 0, (long)0); /* Clear the A-timer interrupt routine. */
    mdaemon_exists = 0;
/*    Setexc(0x102, oldterminate); */
    mdmessagebuf_dtor(msgbuf);
    msgbuf = 0;
    return 0;
    } /* End of function mdaemon_term_wait. */

/*---------------------------------------------------------------------------
mdaemon_time() returns the time that the daemon has run, in mS.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     mdaemon_time     */
/*----------------------*/
long mdaemon_time()
    {
    return mclock;
    } /* End of function mdaemon_time. */

/*---------------------------------------------------------------------------
mdaemon_clock_set() sets the music daemon's clock.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   mdaemon_clock_set  */
/*----------------------*/
void mdaemon_clock_set(t)
    long t;
    {
    mclock = t & 0x7fff;
    } /* End of function mdaemon_clock_set. */

/*---------------------------------------------------------------------------
mdaemon() is the music daemon. It is called every 5 mS,
if it is not already running.
If the clock time has reached the "next-message-time", then a new message
is fetched from the buffer. If one is available, then it is executed.
If the daemon falls behind, it does not send any more instructions to the
MIDI port, but fetches instructions until it catches up, if it can.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       mdaemon        */
/*----------------------*/
static void mdaemon()
    {
    mdmessage msg;
    int toolate = 0;

    /* First check the MIDI input: */


    /* Daemon must wait for the clock to reach "nextmsgtime". */
    if (mclock < nextmsgtime)
        return;

    /* Fetch a saved next instruction, if there is one. */
    if (nextmsg.time) { /* This probably never happens. */
        msg.time = nextmsg.time;
        nextmsg.time = 0;
        }
    else { /* If there is no waiting message, fetch one from the buffer: */
/*        if (mdmessagebuf_get(msgbuf, &msg) != 1) */
        if (mdp_play >= mdp)
            return;
        msg.time = mdmessage_buffer[mdp_play++].time;
        }
    /* Now interpret a message or two: */
    for (;;) {
        toolate = maxdelay && mclock > nextmsgtime + maxdelay;
        if (msg.time < 0) { /* Case of an absolute time instruction. */
            nextmsgtime = msg.time & TIMEMASK;
            }
        else { /* Case of an opcode/args instruction. */
            switch(msg.msg.op) {
            case opNULL:    /* Do-nothing opcode. */
                break;
            case opONE:     /* Send only args[0]. */
                if (!toolate)
                    Midiws(1, msg.msg.args);
                break;
            case opTWO:     /* Send only args[0] and args[1]. */
                if (!toolate)
                    Midiws(2, msg.msg.args);
                break;
            case opTHREE:   /* Send args[0], args[1] and args[2]. */
                if (!toolate)
                    Midiws(3, msg.msg.args);
                break;
            case opDELAY:   /* Make a relative change to "nextmsgtime". */
                nextmsgtime += msg.time & RELTIMEMASK;
                break;
            default:        /* Ignore all other errors. */
                break;
                } /* End of switch. */
            }
        /* Fetch another instruction, and execute it if it is time: */
        if (mclock < nextmsgtime)
            return;
/*        if (mdmessagebuf_get(msgbuf, &msg) != 1)
            return; */
        if (mdp_play >= mdp)
            return;
        msg.time = mdmessage_buffer[mdp_play++].time;
        }
    } /* End of function mdaemon. */

/*------------------------------------------------------------------------------
This is the handler for the 5mS interrupts.
------------------------------------------------------------------------------*/
/*----------------------*/
/*    mdaemon_handler   */
/*----------------------*/
static void mdaemon_handler(sig, code, scp, addr)
    int sig, code;
    struct sigcontext* scp;
    char* addr;
    {
    struct timeval tv;
    struct timezone tz;

    if (clock_on) {
        mclock += 50;
        if ((mclock % 2000) == 0) {
            printf("Handler time = %ld.\n", (long)mclock);
/*            printf("CPU time = %ld microseconds.\n", (long)clock()); */
            printf("mdp = %ld; mdp_play = %ld\n", (long)mdp, (long)mdp_play);
            mdmessage_show(&mdmessage_buffer[mdp_play]);

            if (gettimeofday(&tv, (struct timezone*)0) < 0) {
                perror("mdaemon_handler: gettimeofday()");
                }
            printf("Time of day = %ld.%ld\n", tv.tv_sec, tv.tv_usec);
            }
        }

    /* Prevent two handlers being simultaneously active: */
    /* (But the signal is supposed to be blocked while this signal is
        being handled anyway. So double entry should be impossible.) */
    if (mdaemon_busy) /* Atomic test-and-set is probably not necessary. */
        return;
    mdaemon_busy = 1;

    /* Send some bytes to the MIDI interface: */
    mdaemon();

    mdaemon_busy = 0;
    } /* End of function mdaemon_handler. */

/*---------------------------------------------------------------------------
mdaemon_clock_start() starts the music daemon's clock.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  mdaemon_clock_start */
/*----------------------*/
void mdaemon_clock_start()
    {
    struct itimerval value, ovalue;
    struct sigvec vec, ovec;
    int ret = 0;

    clock_on = 1;

    /* Set the handler for the 50mS timeouts: */
    vec.sv_handler = mdaemon_handler;
    vec.sv_mask = 0;
    vec.sv_flags = 0;
    ret = sigvec((int)SIGALRM, &vec, &ovec);

    value.it_value.tv_sec =         0; /* First time-out interval. */
    value.it_value.tv_usec =    49998; /* First time-out interval. */
    value.it_interval.tv_sec =      0; /* Subsequent time-out intervals. */
    value.it_interval.tv_usec = 49998; /* Subsequent time-out intervals. */

    setitimer((int)ITIMER_REAL, &value, &ovalue);
    } /* End of function mdaemon_clock_start. */

/*---------------------------------------------------------------------------
mdaemon_clock_stop() stops the music daemon's clock.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  mdaemon_clock_stop  */
/*----------------------*/
void mdaemon_clock_stop()
    {
    struct itimerval value, ovalue;
    value.it_value.tv_sec =        0; /* First time-out interval. */
    value.it_value.tv_usec =       0; /* First time-out interval. */
    value.it_interval.tv_sec =     0; /* Subsequent time-out intervals. */
    value.it_interval.tv_usec =    0; /* Subsequent time-out intervals. */

    setitimer((int)ITIMER_REAL, &value, &ovalue);
    clock_on = 0;
    } /* End of function mdaemon_clock_stop. */

/*---------------------------------------------------------------------------
mdaemon_maxdelay() sets the music daemon's maximum delay of sending.
If it reads a message that is more out-of-date than this, it is not sent.
The return value is always the old value of the maximum delay.
If the argument is negative, then the value is not changed.
Otherwise the argument is taken as the new value of the maximum delay.
If the maximum delay is set to 0, this is taken to mean than any delay will
be tolerated.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   mdaemon_maxdelay   */
/*----------------------*/
long mdaemon_maxdelay(d)
    long d;
    {
    long oldmaxdelay;

    if (d < 0)
        return maxdelay;
    oldmaxdelay = maxdelay;
    maxdelay = d;
    return oldmaxdelay;
    } /* End of function mdaemon_maxdelay. */

/*---------------------------------------------------------------------------
This is the new version of this function. Because of having bags of memory on
a sparc station, the whole song is translated into a single message.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  mdaemon_sendbytes_wait  */
/*--------------------------*/
void mdaemon_sendbytes_wait(n, p)
    register int n;
    register char* p;
    {
    mdmessage msg;
    register unsigned char* q;

    if (n <= 0 || !p)
        return;
    while (n > 0) {
        q = &msg.msg.op;
        if (n >= 3) {
            *q++ = opTHREE;
            *q++ = *p++;
            *q++ = *p++;
            *q++ = *p++;
            n -= 3;
            }
        else if (n == 2) {
            *q++ = opTWO;
            *q++ = *p++;
            *q++ = *p++;
            n = 0;
            }
        else {
            *q++ = opONE;
            *q++ = *p++;
            n = 0;
            }
        /* Write the message to the buffer: */
        if (mdp >= mdmessage_buffer_size)
            return;
        mdmessage_buffer[mdp++].time = msg.time;
        }
    } /* End of function mdaemon_sendbytes_wait. */

/*---------------------------------------------------------------------------
mdaemon_sendbytes_wait() puts the given number of bytes into the buffer.
This will hang, if the buffer overflows and a daemon is not active.
Note that the compiler cannot cope with "msg.msg.op = opTHREE" etc.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  mdaemon_sendbytes_wait  */
/*--------------------------*/
/*
void mdaemon_sendbytes_wait(n, p)
    register int n;
    register char* p;
    {
    mdmessage msg;
    register unsigned char* q;

    if (n <= 0 || !p)
        return;
    while (n > 0) {
        q = &msg.msg.op;
        if (n >= 3) {
            *q++ = opTHREE;
            *q++ = *p++;
            *q++ = *p++;
            *q++ = *p++;
            n -= 3;
            }
        else if (n == 2) {
            *q++ = opTWO;
            *q++ = *p++;
            *q++ = *p++;
            n = 0;
            }
        else {
            *q++ = opONE;
            *q++ = *p++;
            n = 0;
            }
        mdmessagebuf_put_wait(msgbuf, &msg);
        }
    } /* End of function mdaemon_sendbytes_wait. */

/*---------------------------------------------------------------------------
mdaemon_senddelay_wait() puts a delay of a given length into the buffer.
This will hang if the buffer overflows and a daemon is not active.
Note that the compiler cannot cope with "msg.msg.op = opTHREE" etc.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  mdaemon_senddelay_wait  */
/*--------------------------*/
void mdaemon_senddelay_wait(l)
    register long l;                /* Delay in mS. */
    {
    mdmessage msg;

    /* If the buffer has overflowed, return: */
    if (mdp >= mdmessage_buffer_size)
        return;

    /* Append a relative time message to the message buffer: */
    msg.time = l & RELTIMEMASK;
    msg.msg.op = opDELAY;

    /* Not necessary to wait, because the buffer is not circular: */
/*    mdmessagebuf_put_wait(msgbuf, &msg); */
    /* Hope that the compiler can do this: */
    mdmessage_buffer[mdp++].time = msg.time;
    } /* End of function mdaemon_senddelay_wait. */

/*---------------------------------------------------------------------------
mdaemon_show() shows the buffer state, for debugging purposes.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     mdaemon_show     */
/*----------------------*/
void mdaemon_show()
    {
    printf("State of the mdaemon buffer:\n");
    printf("Address of buffer: %ld.\n", msgbuf);
    if (msgbuf)
        mdmessagebuf_show(msgbuf);
    printf("\nMIDI input buffer:\n");
/*    if (!midi_in_buffer) {
        midi_in_buffer = Iorec(2);
        if (!midi_in_buffer) {
            printf("Couldn't get MIDI input buffer address.\n");
            return;
            }
        }
    printf("\tBuffer is at 0x%lx\n", (long)midi_in_buffer->ibuf);
    printf("\tBuffer size  = %d\n", midi_in_buffer->ibufsiz);
    printf("\tHead index   = %d\n", midi_in_buffer->ibufhd);
    printf("\tTail index   = %d\n", midi_in_buffer->ibuftl);
    printf("\tLow mark     = %d\n", midi_in_buffer->ibuflow);
    printf("\tHigh mark    = %d\n", midi_in_buffer->ibufhigh);
*/
    } /* End of function mdaemon_show. */

/*---------------------------------------------------------------------------
mdaemon_read_midi_buf() reads the current contents of the MIDI buffer, and
displays them on the screen.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*   mdaemon_read_midi_buf  */
/*--------------------------*/
void mdaemon_read_midi_buf(mode)
    int mode;
    {
    register int nbytes = 0;    /* Number of bytes on the screen so far. */
    register long c;
    register long tim0 = mclock, tim1;

    /* Code and comments copied from song_play(): */
    /* Start up the music daemon - and the clock! (Both very important.) */
    mdaemon_term_wait(); /* Note: Really need mdaemon_reset_wait() here. */
    mdaemon_init();
    mdaemon_clock_start();

    for (;;) {
/*        while (!Bconstat(MIDI_PORT)) {
            if (mode == 0 || Bconstat(CON_PORT)) {
                printf("\n");
                return;
                }
            }
        c = Bconin(MIDI_PORT) & 0xff;
*/
        if (c == 0xfe)
            continue;
        if (c & 0x80) {
            tim1 = mclock - tim0;
            tim0 += tim1;
            if (nbytes > 0) {
                nbytes = 0;
                printf("\n");
                }
            printf("%4ld.%03d: ", (long)(tim1/1000), (int)(tim1%1000));
            }
        if (nbytes >= 16) { /* 16 bytes per line. */
            nbytes = 0;
            printf("\t- \n");
            }
        else {
            if (nbytes > 0)
                printf(" ");
            }
        printf("%02x", (int)c);
        ++nbytes;
        }
    printf("\n");
    } /* End of function mdaemon_read_midi_buf. */

/*---------------------------------------------------------------------------
A_handler() is the interrupt handler called whenever there is an interrupt
by timer A of the MFP 68901 chip.
This routine is written in 68000 assembly language (or a variant thereof).
Because of the danger of overrunning the 5mS time between interrupts, the
semaphore "daemonbusy" is used. If the daemon has already been invoked, a
quick exit is made. Otherwise, "mdaemon" is called in supervisor mode.

What this function does:
void A_handler() {
    if (clock_on) {
        mclock += 5;
        if (mclock < 0)
            mclock = 0;
        }
xxx2:
    if (mdaemon_busy) {
        unsigned char* p = 0xfffa0f;// A register in the MFP.
        *p &= ~(1 << 5);            // Clear bit 5.
        return;
        }
xxx3:
    SR = 0x2300;                    // Set interrupt level mask to 3.
    unsigned char* p = 0xfffa0f;    // A register in the MFP.
    *p &= ~(1 << 5);                // Clear bit 5.
    // Save registers.
    mdaemon();
    // Restore registers.
    mdaemon_busy = 0;
    return;
    } // End of function A_handler.

Notes:
-   Bit 5 of the relevant byte-size register of the MFP has to be cleared so
    as to allow the next timer interrupt to occur. Otherwise the MFP would just
    ignore the next 5mS signal it gets from the timer chip.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       A_handler      */
/*----------------------*/
/* Function removed! */
