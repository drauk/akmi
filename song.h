/* src/akmi/song.h   26 January 1999   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_SONG_H
#define AKMI_SONG_H
/*---------------------------------------------------------------------------
Structs in this file:

span
song
---------------------------------------------------------------------------*/

#include "voice.h"
/* #include "daemon.h"
#include <osbind.h> */
#include "term.h"

typedef struct SPAN {           /* A defined span of events in the song. */
    struct SPAN* next;
    int tag;                    /* The tag-value for the span. */
    voice* span_voice;          /* The voice in which the span is located.*/
    event* first;               /* Address of start-marker */
    event* last;                /* Address of end-marker */
    } span;

typedef struct SONG {
    struct SONG* next;
    uint16 number;          /* For indexing songs. */
    char* title;
    voice* voices;          /* Used for reading and storing the song. */
    voice* currentvoice;    /* Used while reading the song. */
/*  long maxdelay;          /* Probably not very important. */
    double speed_multiplier;/* Speed multiplier. */
    int signature;          /* Key signature: -7 = 7 flats, +7 = 7 sharps. */
    span* spans;            /* List of defined spans. */
    rat* staccato_rat;      /* The staccato factor. */
    rat* note_value;        /* The note value of each unit note. */
    } song;

typedef enum ACCIDENTAL {
    accNONE, accNATURAL, accSHARP, accFLAT, acc2SHARP, acc2FLAT
    } accidental;

extern song*    new_song();
extern void     song_newtitle();
extern void     song_newvoice();
extern void     song_showvoices();
extern voice*   song_findvoice();
extern int      song_currentvoice();
extern void     song_setchannel();
extern void     song_speed_multiplier();
extern void     song_speed_multiply();
extern void     song_keysig();
extern void     song_base();
extern void     song_basenote();
extern void     song_set_program();
extern void     song_set_program_name();
extern void     song_set_voice_name();
extern void     song_voice_pan();
extern void     song_perform_transpose();
extern void     song_compile_transpose();
extern void     song_event_transpose();
extern void     song_max_volume();
extern void     song_event_volume();
extern void     song_event_program();
extern void     song_voice_on_velocity();
extern void     song_voice_off_velocity();
extern void     song_metronome();
extern void     song_event_note_value();
extern void     song_perform_note_value();
extern void     song_note();
extern void     song_rest();
extern void     song_set_marker();
extern void     song_set_repeat();
extern void     song_showevents();
extern void     song_showallevents();
extern void     song_play();
extern void     song_file();

extern song*    songs_find();
extern song*    songs_newsong();
extern int      songs_delremove();
extern void     songs_showsongs();

#endif /* AKMI_SONG_H */
