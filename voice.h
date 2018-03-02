/* src/akmi/voice.h   2009-4-14   Alan Kennington. */
/* $Id$ */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_VOICE_H
#define AKMI_VOICE_H
/*---------------------------------------------------------------------------
Structs/unions/enums in this file:

struct  event
struct  eventlist
struct  voice
---------------------------------------------------------------------------*/

#include "rat.h"
#include <stdio.h>
/* extern char* malloc();      /* There is no malloc.h? */

typedef double metrotype; /* Type used for recording metronome settings. */

typedef enum ETYPE {        /* Parameters: */
    eNULL,
    eMETRO,
    eON,
    eOFF,
    eMARKER,                /* int:    marker tag. */
    eREPEAT,                /* event*: pointer to first marker event. */
    eTRANSPOSE,             /* int:    relative transposition. */
    eCONTROL0,              /* char:   7-bit control number for 0xbn command. */
    eCONTROL1,              /* char[2]: 0xcn 7-bit control number and value. */
    eVOLUME,                /* char:   volume level for 0xb0 7 command. */
    eNOTE_VALUE,            /* struct rat: ratio of akmi to midi 1/4 note. */
    ePROGRAM                /* program change event. */
    } etype;

/* Temporary switch between event time representations: */
#define OLD_TIME

typedef struct EVENT {
    struct EVENT* next;             /* List linkage. */
#ifdef OLD_TIME
    rat time;                       /* Time of execution of event. */
#else
    rat delta;                      /* Experimental time representation. */
#endif
    etype type;
    char params[8];                 /* To be used as a union. */
                                    /* Must be big enough for a rat! */
    } event;

typedef struct EVENTLIST {
    event *first, *last;
    event *current;         /* Last event with time <= current_time. */
    rat current_rat;        /* Current time for event insertion. */
#ifndef OLD_TIME
    rat current_total;      /* Total time up to event *current. */
#endif
    } eventlist;

typedef struct REPEAT_ITEM {
    struct REPEAT_ITEM* next;
    int tag;                    /* Tag of the span being repeated. */
    rat offset;                 /* Offset to be added to the events read. */
    int perform_transpose;      /* Previous voice->event_transpose value. */
    event* nextevent;           /* The event to come back to afterwards. */
    } repeat_item;

typedef struct VOICE {
    struct VOICE* next;             /* List linkage. */
    int v;                          /* Number of the voice. */
    int basenote;                   /* Lowest note of the default octave. */
    int baseoctave12;               /* Octave of default octave. */
    int baseletterC;                /* Letter at base of default octave. */
    int inversion;                  /* Equals 0x0020 if inverting letters. */
    eventlist events;               /* Future event list. */
    rat lasttime;                   /* Time of last event added. */
    rat currenttime;                /* Time of next current insertion. */
    rat next_event_time;            /* Current time while playing. */
    int channel;                    /* Current MIDI channel. */
    int on_velocity;                /* Current note-on velocity. */
    int off_velocity;               /* Current note-off velocity. */
    int program;                    /* MIDI program for the voice. */
    char* program_name;             /* Name of MIDI program. */
    char* name;                     /* Name for MIDI file. */
    int pan;                        /* Pan position of the voice. */
    boolean pan_set;                /* True if pan has been set. */
    event* nextevent;               /* Next event to be played. */
    repeat_item* repeat_stack;      /* Stack of repeat-items. */
    int perform_transpose;          /* Transposition for whole performance.*/
    int compile_transpose;          /* Compile-time transposition. */
    int event_transpose;            /* Temp. perform-time transposition. */
    int max_volume;                 /* Maximum voice volume parameter. */
    long last_mfile_time;           /* For MIDI file output. */
    } voice;

extern event*       new_event();

extern void         eventlist_fprint();

extern void         repeat_item_ctor();
extern repeat_item* new_repeat_item();

extern void         voice_basenote();
extern void         voice_baseoctave();
extern void         voice_baseletter();
extern void         voice_inversion();
extern void         voice_event_metro();
extern void         voice_note();
extern void         voice_rest();
extern event*       voice_set_repeat();
extern event*       voice_set_marker();
extern void         voice_setchannel();
extern void         voice_set_on_velocity();
extern void         voice_set_off_velocity();
extern void         voice_set_program();
extern void         voice_set_program_name();
extern void         voice_set_name();
extern void         voice_pan();
extern void         voice_perform_transpose();
extern void         voice_compile_transpose();
extern event*       voice_event_transpose();
extern void         voice_max_volume();
extern void         voice_event_volume();
extern void         voice_event_note_value();
extern void         voice_event_program();

extern void         voices_dtor();
extern void         voices_delete();
extern voice*       voices_findvoice();
extern voice*       voices_newvoice();
extern void         voices_set_voice_name();
extern void         voices_newmetro();
extern void         voices_new_note_value();
extern void         voices_showvoices();

#endif /* AKMI_VOICE_H */
