/* src/akmi/song.c   4 November 2000   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999-2000, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

c2note

span_ctor
new_span
spans_dtor
spans_delete

song_ctor
new_song
song_dtor
song_delete
song_newtitle
song_newvoice
song_showvoices
song_findvoice
song_currentvoice
song_setchannel
song_speed_multiplier
song_speed_multiply
song_keysig
song_base
song_basenote
song_set_program
song_set_program_name
song_set_voice_name
song_voice_pan
song_perform_transpose
song_compile_transpose
song_event_transpose
song_max_volume
song_event_volume
song_event_program
song_voice_on_velocity
song_voice_off_velocity
song_metronome
song_event_note_value
song_perform_note_value
song_note
song_rest
song_set_marker
song_showevents
song_showallevents
song_play
song_file

songs_find
songs_insertsong
songs_newsong
songs_delremove
songs_showsongs
---------------------------------------------------------------------------*/

#include "song.h"
#include "cblock.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif

/* Default metronome rate is 120 notes/minute. */
#define DEFTMETRONOME 0.5

#define Cb -1
#define C 0
#define Cs 1
#define Db 1
#define D 2
#define Ds 3
#define Eb 3
#define E 4
#define Es 5
#define Fb 4
#define F 5
#define Fs 6
#define Gb 6
#define G 7
#define Gs 8
#define Ab 8
#define A 9
#define As 10
#define Bb 10
#define B 11
#define Bs 12

static int keys[15][7] = {
    {   Cb, Db, Eb, Fb, Gb, Ab, Bb  }, /* Cb */
    {   Cb, Db, Eb, F,  Gb, Ab, Bb  }, /* Gb */
    {   C,  Db, Eb, F,  Gb, Ab, Bb  }, /* Db */
    {   C,  Db, Eb, F,  G,  Ab, Bb  }, /* Ab */
    {   C,  D,  Eb, F,  G,  Ab, Bb  }, /* Eb */
    {   C,  D,  Eb, F,  G,  A,  Bb  }, /* Bb */
    {   C,  D,  E,  F,  G,  A,  Bb  }, /* F  */
    {   C,  D,  E,  F,  G,  A,  B   }, /* C  */
    {   C,  D,  E,  Fs, G,  A,  B   }, /* G  */
    {   Cs, D,  E,  Fs, G,  A,  B   }, /* D  */
    {   Cs, D,  E,  Fs, Gs, A,  B   }, /* A  */
    {   Cs, Ds, E,  Fs, Gs, A,  B   }, /* E  */
    {   Cs, Ds, E,  Fs, Gs, As, B   }, /* B  */
    {   Cs, Ds, Es, Fs, Gs, As, B   }, /* F# */
    {   Cs, Ds, Es, Fs, Gs, As, Bs  }  /* C# */
    };

static int Cscale[7] = {
    C,  D,  E,  F,  G,  A,  B
    };

/*---------------------------------------------------------------------------
c2note() converts a character to a note, using a given signature.
---------------------------------------------------------------------------*/
/*----------------------*/
/*        c2note        */
/*----------------------*/
static int c2note(p, pv, c, acc)
    song* p;
    voice* pv;
    int c;
    int acc;
    {
    register int note, c1;

    if (!p || !pv)
        return -1;
    if (!isascii(c) || !isalpha(c))
        return -1;
    if (acc < 0 || acc >= 6) /* There are precisely 6 kinds of accidental. */
        return -1;

    c ^= pv->inversion;
    c1 = c | 0x20; /* Convert to lower case. */
    if (c1 < 'a' || c1 > 'g')
        return -1;
    c1 -= 'c';
    if (c1 < 0)
        c1 += 7;
    if (acc == (int)accNONE) /* This coercion should not be necessary! */
        note = keys[p->signature + 7][c1];
    else {
        note = Cscale[c1];
        switch(acc) {
        case accFLAT:
            note -= 1;
            break;
        case accSHARP:
            note += 1;
            break;
        case acc2FLAT:
            note -= 2;
            break;
        case acc2SHARP:
            note += 2;
            break;
            }
        }
    note += pv->baseoctave12;
    if (c1 < pv->baseletterC)
        note += 12;
    if (!(c & 0x20)) /* If not lower case... */
        note += 12;
    return note;
    } /* End of function c2note. */

/*---------------------------------------------------------------------------
span_ctor() initialises a "span".
---------------------------------------------------------------------------*/
/*----------------------*/
/*      span_ctor       */
/*----------------------*/
static void span_ctor(p)
    span* p;
    {
    if (!p)
        return;
    p->next = 0;
    p->tag = 0;
    p->span_voice = 0;
    p->first = 0;
    p->last = 0;
    } /* End of function span_ctor. */

/*---------------------------------------------------------------------------
new_span() returns a new "span" structure.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       new_span       */
/*----------------------*/
span* new_span()
    {
    span* p = (span*)malloc(sizeof(span));
    span_ctor(p);
    return p;
    } /* End of function new_span. */

/*---------------------------------------------------------------------------
spans_dtor() destroys a certain kind of list of spans.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      spans_dtor      */
/*----------------------*/
static void spans_dtor(p)
    span** p;
    {
    span *pv1, *pv2;
    if (!p)
        return;
    for (pv1 = *p; pv1; ) {
        pv2 = pv1->next;
/*      span_delete(pv1); */
        free(pv1);
        pv1 = pv2;
        }
    } /* End of function spans_dtor. */

/*---------------------------------------------------------------------------
spans_delete() deletes a "spans" list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     spans_delete     */
/*----------------------*/
static void spans_delete(p)
    span** p;
    {
    if (!p)
        return;
    spans_dtor(p);
    *p = 0;
    } /* End of function spans_delete. */

/*---------------------------------------------------------------------------
song_ctor() initialises a "song".
---------------------------------------------------------------------------*/
/*----------------------*/
/*      song_ctor       */
/*----------------------*/
static void song_ctor(p)
    song* p;
    {
    if (!p)
        return;
    p->next = 0;
    p->number = 0;
    p->title = 0;
    p->voices = 0;
    p->currentvoice = 0;
/*  p->maxdelay = 0; */
    p->speed_multiplier = -1;
    p->signature = 0;   /* No sharps or flats. */
    p->spans = 0;
    p->staccato_rat = 0;
    p->note_value = 0;
    } /* End of function song_ctor. */

/*---------------------------------------------------------------------------
new_song() returns a new "song" structure.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       new_song       */
/*----------------------*/
song* new_song()
    {
    song* p = (song*)malloc(sizeof(song));
    song_ctor(p);
    return p;
    } /* End of function new_song. */

/*---------------------------------------------------------------------------
song_dtor() destroys a "song".
---------------------------------------------------------------------------*/
/*----------------------*/
/*      song_dtor       */
/*----------------------*/
static void song_dtor(p)
    song* p;
    {
    if (!p)
        return;
    free(p->title);
    voices_dtor(p->voices);
    spans_dtor(p->spans);
    if (p->staccato_rat)
        rat_delete(p->staccato_rat);
    } /* End of function song_dtor. */

/*---------------------------------------------------------------------------
song_delete() deletes a "song". To be called only if the song was
allocated on the heap.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      song_delete     */
/*----------------------*/
static void song_delete(p)
    song* p;
    {
    if (!p)
        return;
    song_dtor(p);
    free(p);
    } /* End of function song_delete. */

/*---------------------------------------------------------------------------
song_newtitle() copies the given new title.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     song_newtitle    */
/*----------------------*/
void song_newtitle(p, s)
    song* p;
    char* s;
    {
    char* pc;
    int i;

    if (!p)
        return;
    i = s ? strlen(s) : 0;
    if (p->title)
        free(p->title);
    p->title = 0;
    if (i > 0) {
        p->title = (char*)malloc(i * sizeof(char) + 1);
        if (p->title)
            strcpy(p->title, s);
        }
    } /* End of function song_newtitle. */

/*---------------------------------------------------------------------------
song_newvoice() adds a new voice to the list for the given song.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     song_newvoice    */
/*----------------------*/
void song_newvoice(p, v)
    song* p;
    int v;
    {
    if (!p)
        return;
    voices_newvoice(&p->voices, v);
    } /* End of function song_newvoice. */

/*---------------------------------------------------------------------------
song_showvoices() shows the current list of voices of a song.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    song_showvoices   */
/*----------------------*/
void song_showvoices(p)
    song* p;
    {
    voice* pv;
    if (!p)
        return;
    voices_showvoices(&p->voices);
    pv = p->voices;
    if (p->voices && pv) {
        if (p->currentvoice)
            printf("The current voice is %d.\n", p->currentvoice->v);
        else
            printf("There is no current voice.\n");
        }
    } /* End of function song_showvoices. */

/*----------------------*/
/*    song_findvoice    */
/*----------------------*/
voice* song_findvoice(p, v)
    song* p;
    int v;
    {
    if (!p || v < 0)
        return 0;
    return voices_findvoice(&p->voices, v);
    } /* End of function song_findvoice. */

/*---------------------------------------------------------------------------
song_currentvoice() sets the current voice for future messages.
The return value is the previous voice number, if the previous
voice number was defined. Otherwise, -1 is returned.
If the parameter sent is non-negative, it is used as the current voice
number. Otherwise the current voice number is not changed.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   song_currentvoice  */
/*----------------------*/
int song_currentvoice(p, v)
    song* p;
    int v;
    {
    voice *pv1, *pv2;

    if (!p)
        return -1;
    pv1 = p->currentvoice;
    if (v >= 0) {
        pv2 = voices_findvoice(&p->voices, v);
        if (pv2)
            p->currentvoice = pv2;
        }
    return pv1 ? pv1->v: -1;
    } /* End of function song_currentvoice. */

/*----------------------*/
/*   song_setchannel    */
/*----------------------*/
void song_setchannel(p, c)
    song* p;
    int c;
    {
    if (!p || c < 1 || c > 16 || !p->currentvoice)
        return;
    voice_setchannel(p->currentvoice, c);
    } /* End of function song_setchannel. */

/*---------------------------------------------------------------------------
song_speed_multiplier() sets the speed multiplier of a song to the given
value.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*   song_speed_multiplier  */
/*--------------------------*/
void song_speed_multiplier(p, s)
    song* p;
    double s;
    {
    if (!p || s <= 0)
        return;
    p->speed_multiplier = s;
    } /* End of function song_speed_multiplier. */

/*---------------------------------------------------------------------------
song_speed_multiply() multiplies the current speed multiplier by the given
factor.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  song_speed_multiply */
/*----------------------*/
void song_speed_multiply(p, s)
    song* p;
    double s;
    {
    if (!p || s <= 0)
        return;
    if (p->speed_multiplier > 0)
        p->speed_multiplier *= s;
    else
        p->speed_multiplier = s;
    } /* End of function song_speed_multiply. */

/*---------------------------------------------------------------------------
song_keysig() sets the key signature of a song.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      song_keysig     */
/*----------------------*/
void song_keysig(p, k)
    song* p;
    int k;
    {
    if (!p)
        return;
    if (k < -7 || k > 7)
        return;
    p->signature = k;
    } /* End of function song_keysig. */

/*---------------------------------------------------------------------------
song_base() sets the base letter and octave for the current voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      song_base       */
/*----------------------*/
void song_base(p, c, o)
    song* p;
    int c, o;
    {
    if (!p || !p->currentvoice)
        return;
    c |= 0x20;
    if (c < 'a' || c > 'g')
        return;
    if (o < 0)
        return;
    voice_baseletter(p->currentvoice, c);
    voice_baseoctave(p->currentvoice, o);
    } /* End of function song_base. */

/*---------------------------------------------------------------------------
song_basenote() sets the basenote of the current voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     song_basenote    */
/*----------------------*/
void song_basenote(p, n)
    song* p;
    int n;
    {
    voice* pv;

    if (!p)
        return;
    pv = p->currentvoice;
    if (!pv)
        return;
    voice_basenote(pv, n);
    } /* End of function song_basenote. */

/*---------------------------------------------------------------------------
song_set_program() sets the MIDI program of the current voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   song_set_program   */
/*----------------------*/
void song_set_program(p, n)
    song* p;
    int n;
    {
    voice* pv;

    if (!p)
        return;
    pv = p->currentvoice;
    if (!pv)
        return;
    voice_set_program(pv, n);
    } /* End of function song_set_program. */

/*---------------------------------------------------------------------------
song_set_program_name() sets the MIDI program name of the current voice.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*   song_set_program_name  */
/*--------------------------*/
void song_set_program_name(p, pc)
    song* p;
    char* pc;
    {
    voice* pv;

    if (!p || !pc || !*pc)
        return;
    pv = p->currentvoice;
    if (!pv)
        return;
    voice_set_program_name(pv, pc);
    } /* End of function song_set_program_name. */

/*---------------------------------------------------------------------------
song_set_voice_name() sets the MIDI-file name of a given voice.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*    song_set_voice_name   */
/*--------------------------*/
void song_set_voice_name(p, n, pc)
    song* p;
    long n;         /* The voice to be set. */
    char* pc;
    {
    if (!p || n < 0 || !pc || !*pc)
        return;
    voices_set_voice_name(&p->voices, n, pc);
    } /* End of function song_set_voice_name. */

/*---------------------------------------------------------------------------
song_voice_pan() sets the pan position of the current voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    song_voice_pan    */
/*----------------------*/
void song_voice_pan(p, n)
    song* p;
    int n;
    {
    voice* pv;

    if (!p)
        return;
    pv = p->currentvoice;
    if (!pv)
        return;
    voice_pan(pv, n);
    } /* End of function song_voice_pan. */

/*---------------------------------------------------------------------------
song_perform_transpose() sets the transposition of the current voice.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  song_perform_transpose  */
/*--------------------------*/
void song_perform_transpose(p, n)
    song* p;
    int n;
    {
    voice* pv;

    if (!p)
        return;
    pv = p->currentvoice;
    if (!pv)
        return;
    voice_perform_transpose(pv, n);
    } /* End of function song_perform_transpose. */

/*---------------------------------------------------------------------------
song_compile_transpose() sets the current transposition of the current voice.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  song_compile_transpose  */
/*--------------------------*/
void song_compile_transpose(p, n)
    song* p;
    int n;
    {
    voice* pv;

    if (!p)
        return;
    pv = p->currentvoice;
    if (!pv)
        return;
    voice_compile_transpose(pv, n);
    } /* End of function song_compile_transpose. */

/*---------------------------------------------------------------------------
song_event_transpose() adds an event to the event list for the current
voice, to indicate a change of current transpose level.
---------------------------------------------------------------------------*/
/*----------------------*/
/* song_event_transpose */
/*----------------------*/
void song_event_transpose(p, transpose)
    song* p;
    int transpose;
    {
    if (!p || !p->currentvoice)
        return;
    voice_event_transpose(p->currentvoice, transpose);
    } /* End of function song_event_transpose. */

/*---------------------------------------------------------------------------
song_max_volume() adds an event to the event list for the current voice,
to indicate a change of current volume level.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    song_max_volume   */
/*----------------------*/
void song_max_volume(p, volume)
    song* p;
    int volume;
    {
    if (!p || !p->currentvoice)
        return;
    voice_max_volume(p->currentvoice, volume);
    } /* End of function song_max_volume. */

/*---------------------------------------------------------------------------
song_event_volume() adds an event to the event list for the current voice,
to indicate a change of current volume level.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  song_event_volume   */
/*----------------------*/
void song_event_volume(p, volume)
    song* p;
    int volume;
    {
    if (!p || !p->currentvoice)
        return;
    voice_event_volume(p->currentvoice, volume);
    } /* End of function song_event_volume. */

/*---------------------------------------------------------------------------
song_event_program() adds an event to the event list for the current voice,
to indicate a change of MIDI program for the voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  song_event_program  */
/*----------------------*/
void song_event_program(p, program)
    song* p;
    int program;
    {
    if (!p || !p->currentvoice)
        return;
    voice_event_program(p->currentvoice, program);
    } /* End of function song_event_program. */

/*---------------------------------------------------------------------------
song_voice_on_velocity() sets the current on-velocity of the current voice.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  song_voice_on_velocity  */
/*--------------------------*/
void song_voice_on_velocity(p, n)
    song* p;
    int n;
    {
    voice* pv;

    if (!p)
        return;
    pv = p->currentvoice;
    if (!pv)
        return;
    voice_set_on_velocity(pv, n);
    } /* End of function song_voice_on_velocity. */

/*---------------------------------------------------------------------------
song_voice_off_velocity() sets the current on-velocity of the current voice.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  song_voice_off_velocity */
/*--------------------------*/
void song_voice_off_velocity(p, n)
    song* p;
    int n;
    {
    voice* pv;

    if (!p)
        return;
    pv = p->currentvoice;
    if (!pv)
        return;
    voice_set_off_velocity(pv, n);
    } /* End of function song_voice_off_velocity. */

/*---------------------------------------------------------------------------
song_metronome() adds an event to the event list for voice 0, to indicate
a change of metronome speed.
The time is taken from the current time of the current voice. If there is
no current voice, then time 0 is used.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    song_metronome    */
/*----------------------*/
void song_metronome(p, speed)
    song* p;
    metrotype speed;
    {
    static rat r; /* Should really have a function null_rat(). */

    if (!p)
        return;
    if (p->currentvoice)
        voices_newmetro(&p->voices, &p->currentvoice->lasttime, speed);
    else {
        rat_clear(&r);
        voices_newmetro(&p->voices, &r, speed);
        }
    } /* End of function song_metronome. */

/*---------------------------------------------------------------------------
song_event_note_value() adds an event to the event list for the current voice,
to indicate a change of current volume level.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*   song_event_note_value  */
/*--------------------------*/
void song_event_note_value(p, value)
    song* p;
    rat* value;
    {
    static rat r;

    if (!p || !value)
        return;
    if (p->currentvoice)
        voices_new_note_value(&p->voices, &p->currentvoice->lasttime, value);
    else {
        rat_clear(&r);
        voices_new_note_value(&p->voices, &r, value);
        }
    } /* End of function song_event_note_value. */

/*---------------------------------------------------------------------------
song_perform_note_value() sets the note value for each unit note in a file.
The default is a 1/4 note. A copy is made of the "value" parameter.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  song_perform_note_value */
/*--------------------------*/
void song_perform_note_value(p, value)
    song* p;
    rat* value;
    {
    if (!p || !value)
        return;
    rat_delete(p->note_value);
    p->note_value = new_rat();
    rat_assign_rat(p->note_value, value);
    } /* End of function song_perform_note_value. */

/*---------------------------------------------------------------------------
song_note() puts a note of given length into the default voice.
The given character is translated to a note.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       song_note      */
/*----------------------*/
void song_note(p, length, c, acc, static_mode)
    song* p;
    rat* length;
    int c;
    int acc;
    boolean static_mode;
    {
    voice* pv;
    int note;

    if (!p || !length)
        return;
    pv = p->currentvoice;
    if (!pv || pv->v <= 0)
        return;

    note = c2note(p, pv, c, acc);
    voice_note(pv, length, note, static_mode, p->staccato_rat);
    } /* End of function song_note. */

/*---------------------------------------------------------------------------
song_rest() puts a rest of given length into the default voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       song_rest      */
/*----------------------*/
void song_rest(p, length)
    song* p;
    rat* length;
    {
    voice* pv;

    if (!p || !length)
        return;
    pv = p->currentvoice;
    if (!pv || pv->v < 0) /* Allow rests in voice line 0. */
        return;
    voice_rest(pv, length);
    } /* End of function song_rest. */

/*---------------------------------------------------------------------------
song_set_marker() puts a marker with the given tag into the default voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    song_set_marker   */
/*----------------------*/
void song_set_marker(p, tag)
    song* p;
    int tag;
    {
    span* ps;
    voice* pv;
    event* pe;

    if (!p || tag < 0)
        return;
    for (ps = p->spans; ps; ps = ps->next)
        if (ps->tag == tag)
            break;
    if (ps && ps->last)         /* Attempted triple definition! */
        return;
    pv = p->currentvoice;
    if (!pv || pv->v < 0)
        return;
    if (ps && ps->span_voice != pv) /* Two spans must be in same voice. */
        return;
    pe = voice_set_marker(pv, tag);
    if (!pe)
        return;
    if (ps) {                   /* Second marker with given tag. */
        ps->last = pe;
        return;
        }
    /* First marker with given tag: */
    ps = new_span();
    ps->tag = tag;
    ps->span_voice = pv;
    ps->first = pe;
    ps->next = p->spans;
    p->spans = ps;
    } /* End of function song_set_marker. */

/*---------------------------------------------------------------------------
song_set_repeat() puts a repeat-event into the default voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    song_set_repeat   */
/*----------------------*/
void song_set_repeat(p, tag)
    song* p;
    int tag;
    {
    span* ps;
    voice* pv;
    event* pe;
    rat* pr;

    if (!p || tag < 0)
        return;
    for (ps = p->spans; ps; ps = ps->next)
        if (ps->tag == tag)
            break;
    if (!ps || !ps->first || !ps->last)
        return;
    pv = p->currentvoice;
    if (!pv || pv->v < 0)
        return;
    pe = voice_set_repeat(pv, ps->first);
    if (!pe)
        return;

    /* Move the clock forward by the duration of the span. */
    pr = new_rat();
    rat_assign_rat(pr, &ps->first->time);
    rat_uminus(pr);
    rat_add_rat(pr, &ps->last->time);
    voice_rest(pv, pr);
    rat_delete(pr);
    } /* End of function song_set_repeat. */

/*---------------------------------------------------------------------------
song_showevents() shows the eventlist for a given voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    song_showevents   */
/*----------------------*/
void song_showevents(p, v)
    song* p;
    int v;
    {
    voice* pv;

    if (!p)
        return;
    pv = voices_findvoice(&p->voices, v);
    if (!pv) {
        printf("Could not find voice %d for displaying events.\n", v);
        return;
        }
    printf("Event list for voice %d.\n", v);
    eventlist_fprint(&pv->events, stdout);
    } /* End of function song_showevents. */

/*---------------------------------------------------------------------------
song_showallevents() shows the eventlists of all voices.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*    song_showallevents    */
/*--------------------------*/
void song_showallevents(p)
    song* p;
    {
    voice* pv;

    if (!p)
        return;
    pv = p->voices;
    if (!pv) {
        printf("  -- no events --\n");
        return;
        }
    for ( ; pv; pv = pv->next) {
        printf("Event list for voice %d.\n", pv->v);
        eventlist_fprint(&pv->events, stdout);
        }
    } /* End of function song_showallevents. */

/*---------------------------------------------------------------------------
song_play() plays a song, starting at note "start_rat".
---------------------------------------------------------------------------*/
/*----------------------*/
/*      song_play       */
/*----------------------*/
void song_play(p, start_rat)
    song* p;
    rat* start_rat;
    {
    register voice* pv;             /* Voice list traversal pointer. */
    register voice* firstvoice;     /* Start of voice list. */
    register voice* nextvoice;      /* Next voice to be executed. */
    register event* nextevent;      /* Next event to be executed. */
    rat lastrat;                    /* Last time read in the event list. */
    rat nextrat;                    /* Next time read in the event list. */
    rat* next_event_time;           /* Time of *nextevent. */
    rat next_event_rat;             /* Temp. rat. */
    register double fptime1000;     /* Current floating point time, in mS. */
    long lasttimesent;              /* Last time sent to daemon, in mS. */
    long nexttime;                  /* Next time to send to daemon, in mS. */
    double metronome1000 = DEFTMETRONOME * (double)1000.0;
                                    /* Current note length, in mS. */
    static long headstart1 = 500;   /* Head start given to daemon, in mS. */
    static long headstart2 = 500;   /* Break after sending programs. */
    static long headstart3 = 500;   /* Break after sending pans. */
    boolean eventfound;             /* True if there are some events. */
    static char midibuf[10];        /* Characters to send to the daemon. */
    char* pc;                       /* Temp. pointer. */
    int tag;                        /* Temp. int. */
    repeat_item* pri;               /* Temp. repeat_item. */
    event* pe1;
/*    int int_check_counter = 0;      /* Ctrl-P interrupt checking counter. */
/*    boolean interrupting = false;   /* True when interrupt is detected. */
    boolean transmitting = false;   /* True when start-point is reached. */
    static rat deft_start_rat;      /* Default start-rat. */
    int ii = 0;

    if (!p || !p->voices)
        return;
    firstvoice = p->voices;

    /* Initialise all of the event pointers:
       The members "nextevent", "next_event_time" and "event_transpose"
       of all voices are set to initial values. */
    /* Should really be able to go from anywhere to anywhere in the song! */
    /* Should use a function voice_song_init() for this loop. */
    eventfound = false;
    for (pv = firstvoice; pv; pv = pv->next) {
        pv->nextevent = pv->events.first;   /* Here should start anywhere! */
        if (pv->nextevent)
            eventfound = true;
        rat_clear(&pv->next_event_time); /* Initialise the voice's clock. */
        pv->event_transpose = pv->perform_transpose; /* Init. transpose. */
        }
    if (!eventfound)
        return;

    /* Start up the music daemon - and the clock! (Both very important.) */
/*    mdaemon_term_wait(); /* Note: Really need mdaemon_reset_wait() here. */
    if (mdaemon_init() < 0) {
        printf("song_play: error when calling mdaemon_init.\n");
        return;
        }

    /* From here until mdaemon_clock_start(), at most 1024 bytes may be sent
       to the daemon. Otherwise there will be deadlock!!!!! */
/*  mdaemon_clock_start(); */

    /* Set the start_rat to the default if it is not defined: */
    if (!start_rat) {
        rat_ctor(&deft_start_rat);
        start_rat = &deft_start_rat;
        }

    /* Give the daemon a head start!!! */
    mdaemon_senddelay_wait(headstart1);

    /* Send out all the MIDI voice-program commands: */
    for (pv = firstvoice; pv; pv = pv->next) {
        if (pv->program >= 0) {
            midibuf[0] = 0xc0 | (0x0f & pv->channel);
            midibuf[1] = 0x7f & pv->program;
            mdaemon_sendbytes_wait(2, midibuf);
            }
        }

    /* Give the daemon another rest, just in case! */
    mdaemon_senddelay_wait(headstart2);

    /* Send out all the pan commands: */
    for (pv = firstvoice; pv; pv = pv->next) {
        if (pv->pan_set) {
            midibuf[0] = 0xb0 | (0x0f & pv->channel);
            midibuf[1] = 10;                /* 10 is the code for panpot. */
            midibuf[2] = 0x7f & pv->pan;
            mdaemon_sendbytes_wait(3, midibuf);
            }
        }

    /* Give the daemon yet another rest, just in case! */
    mdaemon_senddelay_wait(headstart3);

    /* Initialise the metronome: */
    /* (The value of metronome1000 is the time in mS of each unit note.) */
    if (p->speed_multiplier > 0)
        metronome1000 /= p->speed_multiplier;

    /* Initialise time variables: */
    rat_clear(&lastrat);                /* Number of note units played = 0. */
    fptime1000 = 0;                     /* Last time played = 0 mS. */
    nexttime = lasttimesent = 0;        /* Integer versions of fptime1000. */

    /* Then execute all of the events: (actually only put them in a buffer) */
    for (;;) {
        /* Find the "next event" with the minimum time: */
        nextvoice = 0;
        for (pv = firstvoice; pv; pv = pv->next) {
            /* Find out what the next event in this voice is: */
            /* Note that "nextevent" is just a temporary here: */
            nextevent = pv->nextevent;
            if (!nextevent)
                continue;

            /* Copy the rat time of the next event in this voice: */
            next_event_time = &nextevent->time;

            /* Correct the event time with the repeat-stack offset: */
            if (pv->repeat_stack) {
                rat_assign_rat(&next_event_rat, next_event_time);
                rat_add_rat(&next_event_rat, &pv->repeat_stack->offset);
                next_event_time = &next_event_rat;
                }

            /* If event is best so far, choose this event to play next: */
            if (!nextvoice || rat_greater_rat(&nextrat, next_event_time)) {
                nextvoice = pv;
                rat_assign_rat(&nextrat, next_event_time);
                }
            /* Guarantee all contemporaneous OFF events are sent first: */
            else if (rat_diff_double(&nextrat, next_event_time) == 0
                        && nextevent->type == eOFF
                        && nextvoice->nextevent->type == eON) {
                nextvoice = pv;
                rat_assign_rat(&nextrat, next_event_time);
                }
            }

        /* If there are no more events to do, the score is finished: */
        if (!nextvoice)
            break;

        /* Update fptime1000, if necessary, using the current metronome: */
        if (rat_greater_rat(&nextrat, &lastrat)) {
/*
            printf("nextrat = ");
            rat_fprint(nextrat, stdout);
            printf("; lastrat = ");
            rat_fprint(lastrat, stdout);
            printf("\n");
*/
            fptime1000 += rat_diff_double(&nextrat, &lastrat) * metronome1000;

            /* Note that "nexttime" is only sent just before real actions. */
            nexttime = (long)fptime1000;
/*
            printf("fptime1000 = %lf; nexttime = %ld\n",
                (double)fptime1000, (long)nexttime);
*/
            rat_assign_rat(&lastrat, &nextrat);
            }

        /* See if we are far enough into the score to start playing: */
        if (!transmitting && !rat_greater_rat(start_rat, &nextrat)) {
            transmitting = true;
            lasttimesent = nexttime;
/*            mdaemon_clock_start(); */
            mdmessage_buffer_play(mdmessage_buffer);
            }
        /* Execute the event. */
        /* (Note that the previous value of nextevent is wrong.) */
        nextevent = nextvoice->nextevent;
        switch(nextevent->type) {
        /* The eMETRO events are guaranteed to come before
           other sorts of events because they are on track 0. */
        case eMETRO:    /* Adjust the metronome. */
            metronome1000 = *(metrotype*)nextevent->params * (double)1000;
            if (p->speed_multiplier > 0)
                metronome1000 /= p->speed_multiplier;
            break;
        case eON:       /* Turn a note on: */
            if (!transmitting)
                break;
            if (nexttime > lasttimesent) {
                mdaemon_senddelay_wait(nexttime - lasttimesent);
                lasttimesent = nexttime;
                }
            midibuf[0] = 0x90 | (0x0f & nextvoice->channel);
            pc = (char*)nextevent->params;
            midibuf[1] = *pc++;             /* Note. */
            if (nextvoice->event_transpose != 0) {
                midibuf[1] += nextvoice->event_transpose;
                midibuf[1] &= 0x7f;
                }
            midibuf[2] = *pc;               /* On-velocity. */
            mdaemon_sendbytes_wait(3, midibuf);
            break;
        case eOFF:      /* Turn a note off: */
            if (!transmitting)
                break;
            if (nexttime > lasttimesent) {
                mdaemon_senddelay_wait(nexttime - lasttimesent);
                lasttimesent = nexttime;
                }
            midibuf[0] = 0x80 | (0x0f & nextvoice->channel);
            pc = (char*)nextevent->params;
            midibuf[1] = *pc++;             /* Note. */
            if (nextvoice->event_transpose != 0) {
                midibuf[1] += nextvoice->event_transpose;
                midibuf[1] &= 0x7f;
                }
            midibuf[2] = *pc;               /* Off-velocity. */
            mdaemon_sendbytes_wait(3, midibuf);
            break;
        case eMARKER:
            /* Ignore the first marker of a pair. */
            if (!nextvoice->repeat_stack)
                break;

            /* Also ignore intervening unrelated markers: */
            tag = *(int*)nextevent->params;
            if (tag != nextvoice->repeat_stack->tag)
                break;

            /* For the second marker, pop the repeat context. */
            pri = nextvoice->repeat_stack;
            nextvoice->repeat_stack = pri->next;

            /* Undo any transposition events found in the span: */
            nextvoice->event_transpose = pri->perform_transpose;
            nextevent = pri->nextevent; /* Change track back again. */
            free(pri);
            break;
        case eREPEAT:   /* Transfer song-playing to a given marker. */
            /* Do sanity check first. Check that param points to a marker: */
            pe1 = *(event**)nextevent->params;
            if (!pe1 || pe1->type != eMARKER)
                break;

            /* Create structure to push on to span stack: */
            pri = new_repeat_item();
            pri->tag = *(int*)pe1->params; /* Copy tag from the marker. */

            /* Save the current transposition for restoration later: */
            pri->perform_transpose = nextvoice->event_transpose;

            /* Let pri->offset = current time - starting epoch of span: */
            rat_assign_rat(&pri->offset, &pe1->time);
            rat_uminus(&pri->offset);
            rat_add_rat(&pri->offset, &nextevent->time);

            /* Add accumulated offset to calculate total jumps made: */
            if (nextvoice->repeat_stack)
                rat_add_rat(&pri->offset, &nextvoice->repeat_stack->offset);

            /* Set the event to return to: */
            pri->nextevent = nextevent;

            /* Push the context onto the stack: */
            pri->next = nextvoice->repeat_stack;
            nextvoice->repeat_stack = pri;

            /* Jump to the beginning of the span. */
            nextevent = pe1;
            break;
        case eTRANSPOSE: /* Event to change the absolute transpose value. */
            tag = *(int*)nextevent->params;
            if (nextvoice->repeat_stack)
                tag += nextvoice->repeat_stack->perform_transpose;
            else
                tag += nextvoice->perform_transpose;
            nextvoice->event_transpose = tag;
            break;
        case eCONTROL0: /* Control change without parameter. */
            midibuf[0] = 0xb0 | (0x0f & nextvoice->channel);
            midibuf[1] = nextevent->params[0] & 0x7f; /* Control number. */
            mdaemon_sendbytes_wait(2, midibuf);
            break;
        case eCONTROL1: /* Control change with parameter. */
            midibuf[0] = 0xb0 | (0x0f & nextvoice->channel);
            pc = (char*)nextevent->params;
            midibuf[1] = *pc++ & 0x7f;          /* Control number. */
            midibuf[2] = *pc & 0x7f;            /* Control value. */
            mdaemon_sendbytes_wait(3, midibuf);
            break;
        case eVOLUME: /* Volume event. */
            if ((tag = nextvoice->max_volume) <= 0)
                break;
            midibuf[0] = 0xb0 | (0x0f & nextvoice->channel);
            midibuf[1] = 7;         /* Roland D-70 manual, page 203. */
            midibuf[2] = ((nextevent->params[0]*tag)/127) & 0x7f;
            mdaemon_sendbytes_wait(3, midibuf);
            break;
        case ePROGRAM: /* Program event. */
            midibuf[0] = 0xc0 | (0x0f & nextvoice->channel);
            midibuf[1] = nextevent->params[0] & 0x7f;
            mdaemon_sendbytes_wait(2, midibuf);
            break;
        case eNULL:     /* Ignore anything that looks unusual. */
        default:
            break;
            }
        /* Move the pointer past the executed event: */
        nextvoice->nextevent = nextevent->next;
        }
/*
    for (ii = 0; ii < 40; ++ii)
        mdmessage_show(&mdmessage_buffer[ii]);
*/
    } /* End of function song_play. */

/*---------------------------------------------------------------------------
song_file() outputs a song to a midi file, starting at note "start_rat".
---------------------------------------------------------------------------*/
/*----------------------*/
/*      song_file       */
/*----------------------*/
void song_file(p, start_ratp)
    song* p;
    rat* start_ratp;
    {
    register voice* pv;             /* Voice list traversal pointer. */
    register voice* firstvoice;     /* Start of voice list. */
    register voice* nextvoice;      /* Next voice to be executed. */
    register event* nextevent;      /* Next event to be executed. */
    rat lastrat;                    /* Last time read in the event list. */
    rat nextrat;                    /* Next time read in the event list. */
    rat* next_event_time;           /* Time of *nextevent. */
    rat next_event_rat;             /* Temp. rat. */
    register double fptime1000;     /* Current floating point time, in mS. */
    long lasttimesent;              /* Last time sent to daemon, in mS. */
    long nexttime;                  /* Next time to send to daemon, in mS. */
    double metronome1000 = DEFTMETRONOME * (double)1000.0;
                                    /* Current note length, in mS. */
    static long headstart1 = 500;   /* Head start given to daemon, in mS. */
    static long headstart2 = 500;   /* Break after sending programs. */
    static long headstart3 = 500;   /* Break after sending pans. */
    boolean eventfound;             /* True if there are some events. */
    static char midibuf[10];        /* Characters to send to the daemon. */
    char* pc;                       /* Temp. pointer. */
    int tag;                        /* Temp. int. */
    repeat_item* pri;               /* Temp. repeat_item. */
    event* pe1;
    boolean transmitting = false;   /* True when start-point is reached. */
    static rat deft_start_rat;      /* Default start-rat. */
    int ii = 0, ii2 = 0;            /* Temporaries. */
    FILE* midi_file = 0;            /* MIDI output file. */
    int n_voices = 0;               /* Number of voices. */
    cblocklist* mfile_buf = 0;      /* Midi file buffer array. */
    static char ad_hoc_buf[200];    /* Ad hoc buffer. */
    int time_divisor = 192;         /* Time units as fraction of 1/4 note. */
    int time_multiplier = 192;      /* Time units as fraction of 1/4 note. */
    long mfile_time = 0;            /* Current time in MIDI-file time units. */
    long delta_mfile_time = 0;      /* Delta time in MIDI-file time units. */
    rat temp_rat;                   /* Temporary rat. */
    rat last_vchange_rat;           /* Last time when note-value was changed. */
    long last_vchange_mfile_time = 0; /* Value of mfile_time at last change. */

    if (!p || !p->voices)
        return;
    firstvoice = p->voices;
    rat_clear(&last_vchange_rat);

    /* Initialise all of the event pointers: */
    /* Should really be able to go from anywhere to anywhere in the song! */
    /* Should use a function voice_song_init() for this loop. */
    eventfound = false;
    n_voices = 0;
    for (pv = firstvoice; pv; pv = pv->next) {
        n_voices += 1;
        pv->nextevent = pv->events.first;   /* Here should start anywhere! */
        if (pv->nextevent)
            eventfound = true;
        rat_clear(&pv->next_event_time); /* Initialise the voice's clock. */
        pv->event_transpose = pv->perform_transpose; /* Init. transpose. */
        }
    if (!eventfound)
        return;

    /* Open the midi file for writing: */
    sprintf(ad_hoc_buf, "s%05u.mid", (unsigned int)p->number);
    midi_file = fopen(ad_hoc_buf, "w");
    if (!midi_file) {
        printf("Error: Could not open file \"%s\" for writing.\n", ad_hoc_buf);
        return;
        }

    /* Write the header chunk: */
    fprintf(midi_file, "MThd");     /* Chunk type. */
    putmint32(midi_file, 6);        /* Number of bytes in this chunk body. */
    putmint16(midi_file, 1);        /* File type 1. */
    putmint16(midi_file, n_voices); /* Number of tracks. */
    putmint16(midi_file, time_divisor); /* Time unit = 1/4-note/time_divisor. */

    /* Note: Cannot write MIDI file tracks until track lengths are known!!! */
    mfile_buf = (cblocklist*)malloc(n_voices * sizeof(cblocklist));
    for (ii = 0; ii < n_voices; ++ii)
        cblocklist_ctor(&mfile_buf[ii]);

    /* Write the sequence number: (is this a per-file thing!) */
    cblocklist_putmint(&mfile_buf[0], 0);
    cblocklist_putmint8(&mfile_buf[0], 0xff);
    cblocklist_putmint8(&mfile_buf[0], 0x00);
    cblocklist_putmint8(&mfile_buf[0], 2);
    cblocklist_putmint16(&mfile_buf[0], p->number);

    /* Copyright notice: */
    sprintf(ad_hoc_buf, "Copyright (C) 200?, Author Name.");
    ii2 = strlen(ad_hoc_buf);
    cblocklist_putmint(&mfile_buf[0], 0);
    cblocklist_putmint8(&mfile_buf[0], 0xff);
    cblocklist_putmint8(&mfile_buf[0], 0x02);
    cblocklist_putmint(&mfile_buf[0], ii2);
    cblocklist_put_bytes(&mfile_buf[0], ad_hoc_buf, ii2);

    /* Put out a text event to describe the track: */
    if (firstvoice->name)
        strcpy(ad_hoc_buf, firstvoice->name);
    else
        sprintf(ad_hoc_buf, "Voice v%d (tempo track)", firstvoice->v);
    ii2 = strlen(ad_hoc_buf);
    cblocklist_putmint(&mfile_buf[0], 0);
    cblocklist_putmint8(&mfile_buf[0], 0xff);
    cblocklist_putmint8(&mfile_buf[0], 0x01);
    cblocklist_putmint(&mfile_buf[0], ii2);
    cblocklist_put_bytes(&mfile_buf[0], ad_hoc_buf, ii2);

    /* The name of the score: */
    sprintf(ad_hoc_buf, "Score %u", (unsigned int)p->number);
    if (p->title)
        sprintf(ad_hoc_buf + strlen(ad_hoc_buf), ": %s", p->title);
    ii2 = strlen(ad_hoc_buf);
    cblocklist_putmint(&mfile_buf[0], 0);
    cblocklist_putmint8(&mfile_buf[0], 0xff);
    cblocklist_putmint8(&mfile_buf[0], 0x03);
    cblocklist_putmint(&mfile_buf[0], ii2);
    cblocklist_put_bytes(&mfile_buf[0], ad_hoc_buf, ii2);

    /* Put various initial things into the MIDI file: */
    for (pv = firstvoice, ii = 0; pv; pv = pv->next, ++ii) {
        /* For each real track... */
        if (ii > 0) {
            /* Put out a text meta-event: */
            int ii2 = 0;
            if (pv->name)
                strcpy(ad_hoc_buf, pv->name);
            else {
                sprintf(ad_hoc_buf, "Voice v%d, MIDI channel = %d",
                    pv->v, pv->channel + 1);
                if (pv->program >= 0) {
                    sprintf(ad_hoc_buf + strlen(ad_hoc_buf), ", program = %d",
                        pv->program + 1);
                    }
                if (pv->pan_set) {
                    sprintf(ad_hoc_buf + strlen(ad_hoc_buf), ", pan = %d",
                        (pv->pan >> 3) - 8);
                    }
                }
            ii2 = strlen(ad_hoc_buf);
            cblocklist_putmint(&mfile_buf[ii], 0);
            cblocklist_putmint8(&mfile_buf[ii], 0xff);
            cblocklist_putmint8(&mfile_buf[ii], 0x01);
            cblocklist_putmint(&mfile_buf[ii], ii2);
            cblocklist_put_bytes(&mfile_buf[ii], ad_hoc_buf, ii2);

            /* Write a key signature: (is this a per-track thing!) */
            cblocklist_putmint(&mfile_buf[ii], 0);
            cblocklist_putmint8(&mfile_buf[ii], 0xff);
            cblocklist_putmint8(&mfile_buf[ii], 0x59);
            cblocklist_putmint8(&mfile_buf[ii], 2);
            cblocklist_putmint8(&mfile_buf[ii], p->signature);
            cblocklist_putmint8(&mfile_buf[ii], 0); /* Assume major key. */

            /* Send out the program change command: */
            if (pv->program >= 0) {
                cblocklist_putmint(&mfile_buf[ii], 0);
                cblocklist_putmint8(&mfile_buf[ii],
                    0xc0 | (0x0f & pv->channel));
                cblocklist_putmint8(&mfile_buf[ii], 0x7f & pv->program);
                }

            /* Send out the pan command: */
            if (pv->pan_set) {
                cblocklist_putmint(&mfile_buf[ii], 0);
                cblocklist_putmint8(&mfile_buf[ii],
                    0xb0 | (0x0f & pv->channel));
                cblocklist_putmint8(&mfile_buf[ii], 10); /* Panpot = 10. */
                cblocklist_putmint8(&mfile_buf[ii], 0x7f & pv->pan);
                }
            }
        }

    /* Default start_ratp (the time to be omitted from beginning of score): */
    if (!start_ratp) {
        rat_ctor(&deft_start_rat);
        start_ratp = &deft_start_rat;
        }

    /* Initialise the metronome: */
    if (p->speed_multiplier > 0)
        metronome1000 /= p->speed_multiplier;

    /* Calculate the multiplier for mfile_time
       as time_multiplier = time_divisor * 4 * note_value. */
    if (rat_positive(p->note_value))
        time_multiplier = rat_mult_int(p->note_value, time_divisor * 4);
    else
        time_multiplier = time_divisor;

    /* Initialise time variables: */
    rat_clear(&lastrat);                /* Rational time (akmi unit notes). */
    fptime1000 = 0;                     /* Floating point time (mS). */
    nexttime = lasttimesent = 0;        /* Times in mS. */

    /* Note that MIDI-file time "mfile_time" = "time_multiplier" * "nextrat".
       But this is not quite true, unless time_multiplier is constant. */
    mfile_time = 0;                     /* Long integer MIDI-file time. */
    delta_mfile_time = 0;               /* Delta-time actually sent to file. */

    for (pv = firstvoice; pv; pv = pv->next)
        pv->last_mfile_time = 0;

    /* Then execute all of the events: */
    for (;;) {
        /* Find the "next event" with the minimum time: */
        int iii = 0, iiii;
        nextvoice = 0;
        for (pv = firstvoice, iiii = 0; pv; pv = pv->next, ++iiii) {
            /* Note that "nextevent" is just a temporary here: */
            nextevent = pv->nextevent;
            if (!nextevent)
                continue;
            next_event_time = &nextevent->time;
            if (pv->repeat_stack) {
                rat_assign_rat(&next_event_rat, next_event_time);
                rat_add_rat(&next_event_rat, &pv->repeat_stack->offset);
                next_event_time = &next_event_rat;
                }
            if (!nextvoice || rat_greater_rat(&nextrat, next_event_time)) {
                nextvoice = pv;
                iii = iiii;
                rat_assign_rat(&nextrat, next_event_time);
                }
            /* Guarantee all contemporaneous OFF events are sent first: */
            else if (rat_diff_double(&nextrat, next_event_time) == 0
                        && nextevent->type == eOFF
                        && nextvoice->nextevent->type == eON) {
                nextvoice = pv;
                iii = iiii;
                rat_assign_rat(&nextrat, next_event_time);
                }
            }
        if (!nextvoice)
            break; /* This happens if there are no more events to do. */

        /* Update fptime1000, if necessary, using the current metronome: */
        if (rat_greater_rat(&nextrat, &lastrat)) {
            rat diff_rat;
            fptime1000 += rat_diff_double(&nextrat, &lastrat) * metronome1000;

            /* Note that "nexttime" is only sent just before real actions. */
            nexttime = (long)fptime1000;
            rat_assign_rat(&lastrat, &nextrat);

            /* Calculate mfile time: */
            rat_assign_rat(&diff_rat, &last_vchange_rat);
            rat_uminus(&diff_rat);
            rat_add_rat(&diff_rat, &nextrat);
            mfile_time = last_vchange_mfile_time
                         + rat_mult_int(&diff_rat, time_multiplier);
            }

        /* Actually ignore the "transmitting" flag. Just do all notes. */
        /* If omitting the first start_ratp notes, turn on transmission now: */
        if (!transmitting && !rat_greater_rat(start_ratp, &nextrat)) {
            transmitting = true;
            lasttimesent = nexttime;
            }

        /* Execute the event. */
        /* (Note that the previous value of nextevent is wrong.) */
        nextevent = nextvoice->nextevent;
        switch(nextevent->type) {
        /* The eMETRO and eNOTE_VALUE events are guaranteed to come before
           other sorts of events because they are on track 0. */
        case eMETRO:    /* Adjust the metronome. */
            metronome1000 = *(metrotype*)nextevent->params * (double)1000;
            if (p->speed_multiplier > 0)
                metronome1000 /= p->speed_multiplier;

            /* Write a tempo change: */
            delta_mfile_time = mfile_time - nextvoice->last_mfile_time;
            cblocklist_putmint(&mfile_buf[0], delta_mfile_time);
            cblocklist_putmint8(&mfile_buf[0], 0xff);
            cblocklist_putmint8(&mfile_buf[0], 0x51);
            cblocklist_putmint8(&mfile_buf[0], 3);
            cblocklist_putmint24(&mfile_buf[0],
                (int)((metronome1000*time_divisor/time_multiplier)*1000));
            if (delta_mfile_time > 0)
                nextvoice->last_mfile_time = mfile_time;
            break;
        case eNOTE_VALUE: { /* Adjust the time multiplier. */
            rat* pr = (rat*)nextevent->params;
            if (!pr || !rat_positive(pr))
                break;
            rat_assign_rat(&temp_rat, pr);

            if (p->note_value && rat_positive(p->note_value))
                rat_mult_rat(&temp_rat, p->note_value);
            time_multiplier = rat_mult_int(&temp_rat, time_divisor * 4);

            /* Write the tempo change: */
            delta_mfile_time = mfile_time - nextvoice->last_mfile_time;
            cblocklist_putmint(&mfile_buf[0], delta_mfile_time);
            cblocklist_putmint8(&mfile_buf[0], 0xff);
            cblocklist_putmint8(&mfile_buf[0], 0x51);
            cblocklist_putmint8(&mfile_buf[0], 3);
            cblocklist_putmint24(&mfile_buf[0],
                (int)((metronome1000*time_divisor/time_multiplier)*1000));
            if (delta_mfile_time > 0)
                nextvoice->last_mfile_time = mfile_time;

            rat_assign_rat(&last_vchange_rat, &nextrat);
            last_vchange_mfile_time = mfile_time;
            }
            break;
        case eON:       /* Turn a note on: */
/*
            if (!transmitting)
                break;
*/
            if (nexttime > lasttimesent)
                lasttimesent = nexttime;

            midibuf[0] = 0x90 | (0x0f & nextvoice->channel);
            pc = (char*)nextevent->params;
            midibuf[1] = *pc++;             /* Note. */
            if (nextvoice->event_transpose != 0) {
                midibuf[1] += nextvoice->event_transpose;
                midibuf[1] &= 0x7f;
                }
            midibuf[2] = *pc;               /* On-velocity. */

            delta_mfile_time = mfile_time - nextvoice->last_mfile_time;
            cblocklist_putmint(&mfile_buf[iii], delta_mfile_time);
            cblocklist_put_bytes(&mfile_buf[iii], midibuf, 3);
            if (delta_mfile_time > 0)
                nextvoice->last_mfile_time = mfile_time;
            break;
        case eOFF:      /* Turn a note off: */
/*
            if (!transmitting)
                break;
*/
            if (nexttime > lasttimesent)
                lasttimesent = nexttime;

            midibuf[0] = 0x80 | (0x0f & nextvoice->channel);
            pc = (char*)nextevent->params;
            midibuf[1] = *pc++;             /* Note. */
            if (nextvoice->event_transpose != 0) {
                midibuf[1] += nextvoice->event_transpose;
                midibuf[1] &= 0x7f;
                }
            midibuf[2] = *pc;               /* Off-velocity. */

            delta_mfile_time = mfile_time - nextvoice->last_mfile_time;
            cblocklist_putmint(&mfile_buf[iii], delta_mfile_time);
            cblocklist_put_bytes(&mfile_buf[iii], midibuf, 3);
            if (delta_mfile_time > 0)
                nextvoice->last_mfile_time = mfile_time;
            break;
        case eMARKER:
            /* Ignore the first marker of a pair. */
            if (!nextvoice->repeat_stack)
                break;

            /* Also ignore intervening unrelated markers: */
            tag = *(int*)nextevent->params;
            if (tag != nextvoice->repeat_stack->tag)
                break;

            /* For the second marker, pop the repeat context. */
            pri = nextvoice->repeat_stack;
            nextvoice->repeat_stack = pri->next;

            /* Undo any transposition events found in the span: */
            nextvoice->event_transpose = pri->perform_transpose;
            nextevent = pri->nextevent; /* Change track back again. */
            free(pri);
            break;
        case eREPEAT:   /* Transfer song-playing to a given marker. */
            /* Do sanity check first. Check that param points to a marker: */
            pe1 = *(event**)nextevent->params;
            if (!pe1 || pe1->type != eMARKER)
                break;

            /* Create structure to push on to span stack: */
            pri = new_repeat_item();
            pri->tag = *(int*)pe1->params; /* Copy tag from the marker. */

            /* Save the current transposition for restoration later: */
            pri->perform_transpose = nextvoice->event_transpose;

            /* Let pri->offset = current time - starting epoch of span: */
            rat_assign_rat(&pri->offset, &pe1->time);
            rat_uminus(&pri->offset);
            rat_add_rat(&pri->offset, &nextevent->time);

            /* Add accumulated offset to calculate total jumps made: */
            if (nextvoice->repeat_stack)
                rat_add_rat(&pri->offset, &nextvoice->repeat_stack->offset);

            /* Set the event to return to: */
            pri->nextevent = nextevent;

            /* Push the context onto the stack: */
            pri->next = nextvoice->repeat_stack;
            nextvoice->repeat_stack = pri;

            /* Jump to the beginning of the span. */
            nextevent = pe1;
            break;
        case eTRANSPOSE: /* Event to change the absolute transpose value. */
            tag = *(int*)nextevent->params;
            if (nextvoice->repeat_stack)
                tag += nextvoice->repeat_stack->perform_transpose;
            else
                tag += nextvoice->perform_transpose;
            nextvoice->event_transpose = tag;
            break;
        case eCONTROL0: /* Control change without parameter. */
            midibuf[0] = 0xb0 | (0x0f & nextvoice->channel);
            midibuf[1] = nextevent->params[iii] & 0x7f; /* Control number. */

            delta_mfile_time = mfile_time - nextvoice->last_mfile_time;
            cblocklist_putmint(&mfile_buf[iii], delta_mfile_time);
            cblocklist_put_bytes(&mfile_buf[iii], midibuf, 2);
            if (delta_mfile_time > 0)
                nextvoice->last_mfile_time = mfile_time;
            break;
        case eCONTROL1: /* Control change with parameter. */
            midibuf[0] = 0xb0 | (0x0f & nextvoice->channel);
            pc = (char*)nextevent->params;
            midibuf[1] = *pc++ & 0x7f;          /* Control number. */
            midibuf[2] = *pc & 0x7f;            /* Control value. */

            delta_mfile_time = mfile_time - nextvoice->last_mfile_time;
            cblocklist_putmint(&mfile_buf[iii], delta_mfile_time);
            cblocklist_put_bytes(&mfile_buf[iii], midibuf, 3);
            if (delta_mfile_time > 0)
                nextvoice->last_mfile_time = mfile_time;
            break;
        case eVOLUME: /* Volume event. */
            if ((tag = nextvoice->max_volume) <= 0)
                break;
            midibuf[0] = 0xb0 | (0x0f & nextvoice->channel);
            midibuf[1] = 7;         /* Roland D-70 manual, page 203. */
            midibuf[2] = ((nextevent->params[0]*tag)/127) & 0x7f;

            delta_mfile_time = mfile_time - nextvoice->last_mfile_time;
            cblocklist_putmint(&mfile_buf[iii], delta_mfile_time);
            cblocklist_put_bytes(&mfile_buf[iii], midibuf, 3);
            if (delta_mfile_time > 0)
                nextvoice->last_mfile_time = mfile_time;
            break;
        case ePROGRAM: /* Program-change event. */
            midibuf[0] = 0xc0 | (0x0f & nextvoice->channel);
            midibuf[1] = nextevent->params[0] & 0x7f;

            delta_mfile_time = mfile_time - nextvoice->last_mfile_time;
            cblocklist_putmint(&mfile_buf[iii], delta_mfile_time);
            cblocklist_put_bytes(&mfile_buf[iii], midibuf, 2);
            if (delta_mfile_time > 0)
                nextvoice->last_mfile_time = mfile_time;
            break;
        case eNULL:     /* Ignore anything that looks unusual. */
        default:
            break;
            }
        /* Move the pointer past the executed event: */
        nextvoice->nextevent = nextevent->next;
        }

    /* Write the while lot out to the MIDI file: */
    for (pv = firstvoice, ii = 0; pv; pv = pv->next, ++ii) {
        int n_bytes;

        /* Append the "end of track" meta-event: */
        cblocklist_putmint8(&mfile_buf[ii], 0);
        cblocklist_putmint8(&mfile_buf[ii], 0xff);
        cblocklist_putmint8(&mfile_buf[ii], 0x2f);
        cblocklist_putmint8(&mfile_buf[ii], 0x00);

        /* Put the file out to the file: */
        n_bytes = cblocklist_length(&mfile_buf[ii]);
        if (n_bytes != mfile_buf[ii].length)
            fprintf(stderr, "Warning: cblocklist::length incorrect.\n");
        fprintf(midi_file, "MTrk");
        putmint32(midi_file, n_bytes);
        cblocklist_print(&mfile_buf[ii], midi_file);
        }
    fclose(midi_file);
    } /* End of function song_file. */

/*---------------------------------------------------------------------------
songs_find() attempts to find a song with a given number.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      songs_find      */
/*----------------------*/
song* songs_find(p, i)
    song** p;
    unsigned int i;
    {
    song* ps;

    if (!p)
        return 0;
    for (ps = *p; ps; ps = ps->next)
        if (ps->number == i)
            break;
    return ps;
    } /* End of function songs_find. */

/*---------------------------------------------------------------------------
songs_insertsong() inserts a given song *ps2 into the list *p, just
after the element *ps1 of the list. If ps1 == 0, then *ps2 is inserted at
the beginning of the list. No check is made of whether ps1 really points to
an element of the list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  songs_insertsong    */
/*----------------------*/
static void songs_insertsong(p, ps1, ps2)
    song** p;
    song *ps1, *ps2;
    {
    if (!p || !ps2)
        return;
    if (!ps1) {
        ps2->next = *p;
        *p = ps2;
        return;
        }
    ps2->next = ps1->next;
    ps1->next = ps2;
    } /* End of function songs_insertsong. */

/*---------------------------------------------------------------------------
songs_newsong() creates a new "song" with a given index, and adds it to
the given list. If anything goes wrong, null is returned. A song may not
have a negative index. Nor may two songs have the same index.
Songs are inserted in the list in increasing order of index.
Note that in future, this routine should be modified to accept a new song
with a duplicate song number, deleting the previous song.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     songs_newsong    */
/*----------------------*/
song* songs_newsong(p, i)
    song** p;
    unsigned int i;
    {
    register song *ps1, *ps2;

    if (!p)
        return 0;
    for (ps1 = *p, ps2 = 0; ps1; ps1 = ps1->next) {
        if (ps1->number >= i)
            break;
        ps2 = ps1;
        }
    if (ps1 && ps1->number == i) /* No two songs may have the same number.*/
        return 0;
    ps1 = new_song();
    if (!ps1)
        return 0;
    ps1->number = i;
    songs_insertsong(p, ps2, ps1);

    return ps1;
    } /* End of function songs_newsong. */

/*---------------------------------------------------------------------------
songs_delremove() removes a song from a list of songs. It then deletes all
memory used by that song.
Return values:
0   all okay
-1  null list pointer
-2  song not found
---------------------------------------------------------------------------*/
/*----------------------*/
/*    songs_delremove   */
/*----------------------*/
int songs_delremove(pp, i)
    song** pp;
    unsigned int i;
    {
    song *p, *q;

    if (!pp)
        return -1;
    for (p = *pp, q = 0; p; p = p->next) {
        if (p->number == i)
            break;
        q = p;
        }
    if (!p)
        return -2;
    /* Remove the song: */
    if (!q) { /* The song was the first element of the list. */
        *pp = p->next;
        }
    else {
        q->next = p->next;
        }
    song_delete(p);
    return 0;
    } /* End of function songs_delremove. */

/*---------------------------------------------------------------------------
songs_showsongs() shows the numbers and titles of a set of songs to stdout.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   songs_showsongs    */
/*----------------------*/
void songs_showsongs(pp)
    song** pp;
    {
    register song* p;

    if (!pp)
        return;
    if (!*pp) {
        printf("The list of songs is empty.\n");
        return;
        }
    printf("List of songs:\n");
    for (p = *pp; p; p = p->next)
        printf("%u\t%s\n", p->number, p->title ? p->title : "No title");
    } /* End of function songs_showsongs. */
