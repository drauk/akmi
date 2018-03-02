/* src/akmi/voice.c   2009-4-14   Alan Kennington. */
/* $Id$ */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

event_ctor
new_event
event_dtor
event_delete

eventlist_ctor
eventlist_dtor
eventlist_prepend
eventlist_append
eventlist_insert
eventlist_addevent
eventlist_current_insert
eventlist_current_addevent
eventlist_current_time
eventlist_fprint

repeat_item_ctor
new_repeat_item
repeat_item_dtor

voice_ctor
new_voice
voice_dtor
voice_delete
voice_basenote
voice_baseoctave
voice_baseletter
voice_inversion
voice_event_metro
voice_note
voice_rest
voice_set_marker
voice_set_repeat
voice_setchannel
voice_set_on_velocity
voice_set_off_velocity
voice_set_program
voice_set_program_name
voice_set_name
voice_pan
voice_perform_transpose
voice_compile_transpose
voice_event_transpose
voice_max_volume
voice_event_volume
voice_event_note_value
voice_event_program

voices_dtor
voices_delete
voices_findvoice
voices_insertvoice
voices_newvoice
voices_getvoice
voices_set_voice_name
voices_newmetro
voices_new_note_value
voices_newnote
voices_showvoices
---------------------------------------------------------------------------*/

#include "voice.h"
#include "bmem.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

bmem eventmem; /* In C, this has to be initialised by main()! */

char* etype_string[] = { /* Must correspond to enum etype in "message.h". */
    "NULL",
    "METRO",
    "ON",
    "OFF",
    "MARKER",
    "REPEAT",
    "TRANSPOSE",
    "CONTROL0",
    "CONTROL1",
    "VOLUME",
    "NOTE VALUE",
    "PROGRAM"
    };

/*---------------------------------------------------------------------------
event_ctor() initialises an "event".
---------------------------------------------------------------------------*/
/*----------------------*/
/*      event_ctor      */
/*----------------------*/
static void event_ctor(p)
    event* p;
    {
    if (!p)
        return;
    p->next = 0;
#ifdef OLD_TIME
    rat_ctor(&p->time);
#else
    rat_ctor(&p->delta);
#endif
    p->type = eNULL;
    } /* End of function event_ctor. */

/*---------------------------------------------------------------------------
new_event() returns a new event.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      new_event       */
/*----------------------*/
event* new_event()
    {
    event* p;

/*  p = (event*)malloc(sizeof(event)); */
    p = (event*)bmem_newchunk(&eventmem);
    event_ctor(p);
    return p;
    } /* End of function new_event. */

/*---------------------------------------------------------------------------
event_dtor() destroys an "event".
---------------------------------------------------------------------------*/
/*----------------------*/
/*      event_dtor      */
/*----------------------*/
static void event_dtor(p)
    event* p;
    {
    if (!p)
        return;
#ifdef OLD_TIME
/*  rat_dtor(&p->time); /* Unnecessary - no memory allocated by rat. */
#else
/*  rat_dtor(&p->delta); /* Unnecessary - no memory allocated by rat. */
#endif
    } /* End of function event_dtor. */

/*---------------------------------------------------------------------------
event_delete() deletes an "event".
---------------------------------------------------------------------------*/
/*----------------------*/
/*     event_delete     */
/*----------------------*/
static void event_delete(p)
    event* p;
    {
    if (!p)
        return;
    event_dtor(p);
    bmem_freechunk(&eventmem, p);
    } /* End of function event_delete. */

/*---------------------------------------------------------------------------
eventlist_ctor() initialises an "eventlist".
---------------------------------------------------------------------------*/
/*----------------------*/
/*    eventlist_ctor    */
/*----------------------*/
static void eventlist_ctor(p)
    eventlist* p;
    {
    if (!p)
        return;
    p->first = 0;
    p->last = 0;
    p->current = 0;
    rat_ctor(&p->current_rat);
#ifndef OLD_TIME
    rat_ctor(&p->current_total);
#endif
    } /* End of function eventlist_ctor. */

/*---------------------------------------------------------------------------
eventlist_dtor() destroys an "eventlist".
WARNING: This routine is WRONG!!! The memory has to be given back to
bmem, not to malloc via free()!!!
---------------------------------------------------------------------------*/
/*----------------------*/
/*    eventlist_dtor    */
/*----------------------*/
static void eventlist_dtor(p)
    eventlist* p;
    {
    event *pe1, *pe2;

    if (!p)
        return;
    for (pe1 = p->first; pe1; ) {
        pe2 = pe1->next;
        event_delete(pe1);
        pe1 = pe2;
        }
    } /* End of function eventlist_dtor. */

/*---------------------------------------------------------------------------
eventlist_prepend() prepends a given event to a given list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   eventlist_prepend  */
/*----------------------*/
static void eventlist_prepend(p, pe)
    eventlist* p;
    event* pe;
    {
    if (!p || !pe)
        return;
    pe->next = p->first;
    p->first = pe;
    if (!p->last) { /* Case of the formerly empty list. */
        p->last = pe;
#ifdef OLD_TIME
        if (!rat_greater_rat(&pe->time, &p->current_rat))
            p->current = pe;
#else
        if (!rat_greater_rat(&pe->delta, &p->current_rat)) {
            p->current = pe;
            rat_assign_rat(&p->current_total, &pe->delta);
            }
#endif
        }
    } /* End of function eventlist_prepend. */

/*---------------------------------------------------------------------------
eventlist_append() appends a given event to a given list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   eventlist_append   */
/*----------------------*/
static void eventlist_append(p, pe)
    eventlist* p;
    event* pe;
    {
    if (!p || !pe)
        return;
    pe->next = 0;
    if (p->last)
        p->last->next = pe;
    else { /* Case of the empty list. */
        p->first = pe;
#ifdef OLD_TIME
        if (!rat_greater_rat(&pe->time, &p->current_rat))
            p->current = pe;
#else
        if (!rat_greater_rat(&pe->delta, &p->current_rat)) {
            p->current = pe;
            rat_assign_rat(&p->current_total, &pe->delta);
            }
#endif
        }
    p->last = pe;
    } /* End of function eventlist_append. */

/*---------------------------------------------------------------------------
eventlist_insert() inserts a given event *pe2 into the list *p, just
after the element *pe1 of the list. If pe1 == 0, then *pe2 is inserted at the
beginning of the list. No check is made of whether pe1 really points to an
element of the list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  eventlist_insert    */
/*----------------------*/
static void eventlist_insert(p, pe1, pe2)
    register eventlist* p;
    register event *pe1, *pe2;
    {
    if (!p || !pe2)
        return;
    if (!pe1) { /* Insert at beginning of list. */
        pe2->next = p->first;
        p->first = pe2;
        if (!p->last) { /* Empty list case. */
            p->last = pe2;
#ifdef OLD_TIME
            if (!rat_greater_rat(&pe2->time, &p->current_rat))
                p->current = pe2;
#else
            if (!rat_greater_rat(&pe2->delta, &p->current_rat)) {
                p->current = pe2;
                rat_assign_rat(&p->current_total, &pe2->delta);
                }
#endif
            }
        return;
        }
    pe2->next = pe1->next;
    pe1->next = pe2;
    if (pe1 == p->last)
        p->last = pe2;
    } /* End of function eventlist_insert. */

/*---------------------------------------------------------------------------
eventlist_addevent() adds an event to an event list, maintaining
chronological sequence.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  eventlist_addevent  */
/*----------------------*/
#ifdef OLD_TIME
static void eventlist_addevent(p, pe)
#else
static void eventlist_addevent(p, pe, time)
#endif
    eventlist* p;
    event* pe;
#ifndef OLD_TIME
    rat* time;  /* Insertion time. */
#endif
    {
    register event *pe1, *pe2;
#ifdef OLD_TIME
    register rat* time;
    if (!p || !pe)
        return;
    /* Find an event with a larger time: */
    time = &pe->time;
#else
    rat total;
    if (!p || !pe || !time)
        return;
    /* Find an event with a larger time: */
    rat_ctor(&total);
#endif
    for (pe1 = p->first, pe2 = 0; pe1; pe1 = pe1->next) {
#ifdef OLD_TIME
        if (rat_greater_rat(&pe1->time, time))
            break;
#else
        rat_add_rat(&total, &pe1->delta);
        if (rat_greater_rat(&total, time))
            break;
#endif
        pe2 = pe1;
        }
    eventlist_insert(p, pe2, pe);
    } /* End of function eventlist_addevent. */

/*---------------------------------------------------------------------------
eventlist_current_insert() inserts a given event *pe2 into the list *p, just
after the element *current of the list. The pointer "current" is updated.
This function is really an optimised version of eventlist_addevent.
---------------------------------------------------------------------------*/
/*--------------------------*/
/* eventlist_current_insert */
/*--------------------------*/
static void eventlist_current_insert(p, pe2)
    register eventlist* p;
    register event *pe2;
    {
#ifndef OLD_TIME
    rat current_total;
#endif
    if (!p || !pe2)
        return;
    if (!p->current) { /* Insert at beginning of list. */
        pe2->next = p->first;
        p->first = pe2;
        if (!p->last)
            p->last = p->first;
#ifdef OLD_TIME
        if (!rat_greater_rat(&pe2->time, &p->current_rat))
            p->current = pe2;
#else
        if (!rat_greater_rat(&pe2->delta, &p->current_rat)) {
            p->current = pe2;
            rat_assign_rat(&p->current_total, &pe2->delta);
            }
#endif
        return;
        }
    pe2->next = p->current->next;
    p->current->next = pe2;
    if (p->current == p->last)
        p->last = pe2;
#ifdef OLD_TIME
    if (!rat_greater_rat(&pe2->time, &p->current_rat))
        p->current = pe2;
#else
    rat_assign_rat(&current_total, &p->current_total);
    rat_add_rat(&current_total, &pe2->delta);
    if (!rat_greater_rat(&current_total, &p->current_rat)) {
        p->current = pe2;
        rat_assign_rat(&p->current_total, &current_total);
        }
#endif
    } /* End of function eventlist_current_insert. */

/*---------------------------------------------------------------------------
eventlist_current_addevent() adds an event to an event list, maintaining
chronological sequence. The function is effectively optimised by the fact
that the caller is assumed to be sending an event which is not earlier than
the current event *current in the list.
---------------------------------------------------------------------------*/
/*------------------------------*/
/*  eventlist_current_addevent  */
/*------------------------------*/
static void eventlist_current_addevent(p, pe)
    eventlist* p;
    event* pe;
    {
    register rat* pr;
    register event *pe1, *pe2;

    if (!p || !pe)
        return;
    if (!p->first) { /* Empty list case. */
        pe->next = 0;
        p->first = pe;
        p->last = pe;
        if (!rat_greater_rat(&pe->time, &p->current_rat))
            p->current = pe;
        return;
        }
    /* Find an event with a larger time: */
    pr = &pe->time;
    for (pe1 = p->current, pe2 = p->current; pe1; pe1 = pe1->next) {
        if (rat_greater_rat(&pe1->time, pr))
            break;
        pe2 = pe1;
        }
    eventlist_insert(p, pe2, pe);
    } /* End of function eventlist_current_addevent. */

/*---------------------------------------------------------------------------
eventlist_current_time() updates the current time of the list and also
the "current" pointer. This helps to optimise the eventlist_current insertion
functions.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  eventlist_current_time  */
/*--------------------------*/
static void eventlist_current_time(p, time)
    eventlist* p;
    rat* time;
    {
    register rat* pr;
    register event *pe1, *pe2;

    if (!p || !time || rat_greater_rat(&p->current_rat, time))
        return;
    rat_assign_rat(&p->current_rat, time);

    /* Move the "current" pointer. Find an event with a larger time: */
    pe1 = p->current;
    if (!pe1)
        pe1 = p->first;
    for ( ; pe1; pe1 = pe1->next) {
        if (rat_greater_rat(&pe1->time, time))
            break;
        p->current = pe1;
        }
    } /* End of function eventlist_current_time. */

/*---------------------------------------------------------------------------
eventlist_fprint() prints a description of a given event list to a given
file.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   eventlist_fprint   */
/*----------------------*/
void eventlist_fprint(p, f)
    eventlist* p;
    FILE* f;
    {
    event* pe;

    if (!p || !f)
        return;
    if (!p->first) {
        fprintf(f, "  -- no events --\n");
        return;
        }
    fprintf(f, "time\ttype\tparams (hexadecimal)\n");
    for (pe = p->first; pe; pe = pe->next) {
        rat_fprint(&pe->time, f);
        fprintf(f, "\t%s\t%08lx %08lx", etype_string[pe->type],
                *(long*)pe->params, *(long*)&pe->params[4]);
        switch(pe->type) {
        case eMETRO:
            fprintf(f, "\t%lf", *(double*)pe->params);
            break;
        case eON:
        case eOFF:
            fprintf(f, "\t%d", *(char*)pe->params);
            break;
        case eMARKER:
            fprintf(f, "\t%d", *(int*)pe->params);
            break;
        case eCONTROL0:
            fprintf(f, "\t%d", (int)pe->params[0]);
            break;
        case eCONTROL1:
            fprintf(f, "\t%d -> %d", (int)pe->params[0], (int)pe->params[1]);
            break;
        case eVOLUME:
            fprintf(f, "\t%d", (int)pe->params[0]);
            break;
        case eNOTE_VALUE:
            fprintf(f, "\t");
            rat_fprint((rat*)pe->params, f);
            break;
        case ePROGRAM:
            fprintf(f, "\t%d", (int)pe->params[0]);
            break;
        default:
            break;
            } /* End of switch(pe->type). */
        fprintf(f, "\n");
        }
    } /* End of function eventlist_fprint. */

/*---------------------------------------------------------------------------
repeat_item_ctor() initialises a "repeat_item".
---------------------------------------------------------------------------*/
/*----------------------*/
/*   repeat_item_ctor   */
/*----------------------*/
void repeat_item_ctor(p)
    repeat_item* p;
    {
    if (!p)
        return;
    p->next = 0;
    p->tag = 0;
    p->perform_transpose = 0;
    rat_ctor(&p->offset);
    p->nextevent = 0;
    } /* End of function repeat_item_ctor. */

/*---------------------------------------------------------------------------
new_repeat_item() returns a pointer to a new "repeat_item".
---------------------------------------------------------------------------*/
/*----------------------*/
/*    new_repeat_item   */
/*----------------------*/
repeat_item* new_repeat_item()
    {
    repeat_item* p = (repeat_item*)malloc(sizeof(repeat_item));
    if (!p)
        return 0;
    repeat_item_ctor(p);
    return p;
    } /* End of function new_repeat_item. */

/*---------------------------------------------------------------------------
repeat_item_dtor() destroys a "repeat_item". This function doesn't do much!
---------------------------------------------------------------------------*/
/*----------------------*/
/*   repeat_item_dtor   */
/*----------------------*/
static void repeat_item_dtor(p)
    repeat_item* p;
    {
    if (!p)
        return;
    } /* End of function repeat_item_dtor. */

/*---------------------------------------------------------------------------
voice_ctor() initialises a "voice".
---------------------------------------------------------------------------*/
/*----------------------*/
/*      voice_ctor      */
/*----------------------*/
static void voice_ctor(p)
    voice* p;
    {
    if (!p)
        return;
    p->next = 0;
    p->v = -1;              /* Forces the user to set the voice number. */
    p->basenote = 60;       /* Default range is c5 to b5. */
    p->baseoctave12 = 60;   /* Base octave * 12. */
    p->baseletterC = 0;     /* Base letter, relative to C. */
    p->inversion = 0x0000;
    eventlist_ctor(&p->events);
    rat_ctor(&p->lasttime);
    rat_ctor(&p->currenttime);
    rat_ctor(&p->next_event_time);
    p->channel = 0;             /* MIDI channels go from 0 to 15. */
    p->on_velocity = 0x40;      /* Note-on vel. goes from 0x00 to 0x7f. */
    p->off_velocity = 0x40;     /* Note-off vel. goes from 0x00 to 0x7f. */
    p->program = -1;            /* Program goes from 0x00 to 0x7f. */
    p->program_name = 0;        /* Program name string. */
    p->name = 0;                /* MIDI-file name string. */
    p->pan = 0x40;              /* Pan goes from 0x00 to 0x7f. */
    p->pan_set = false;
    p->nextevent = 0;           /* Used while playing a song. */
    p->repeat_stack = 0;
    p->perform_transpose = 0;
    p->compile_transpose = 0;
    p->event_transpose = 0;
    p->max_volume = 0;          /* Default is to disable volume events. */
    p->last_mfile_time = 0;
    } /* End of function voice_ctor. */

/*---------------------------------------------------------------------------
new_voice() returns a pointer to a new "voice".
---------------------------------------------------------------------------*/
/*----------------------*/
/*       new_voice      */
/*----------------------*/
static voice* new_voice()
    {
    voice* p = (voice*)malloc(sizeof(voice));
    if (!p)
        return 0;
    voice_ctor(p);
    return p;
    } /* End of function new_voice. */

/*---------------------------------------------------------------------------
voice_dtor() destroys a "voice".
---------------------------------------------------------------------------*/
/*----------------------*/
/*      voice_dtor      */
/*----------------------*/
static void voice_dtor(p)
    voice* p;
    {
    if (!p)
        return;
    eventlist_dtor(&p->events);
    } /* End of function voice_dtor. */

/*---------------------------------------------------------------------------
voice_delete() deletes a "voice". To be called only if the voice was
allocated on the heap.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     voice_delete     */
/*----------------------*/
static void voice_delete(p)
    voice* p;
    {
    if (!p)
        return;
    voice_dtor(p);
    free(p);
    } /* End of function voice_delete. */

/*---------------------------------------------------------------------------
voice_basenote() sets the basenote of a voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    voice_basenote    */
/*----------------------*/
void voice_basenote(p, n)
    voice* p;
    int n;
    {
    if (!p)
        return;
    p->basenote = n;
    } /* End of function voice_basenote. */

/*---------------------------------------------------------------------------
voice_baseoctave() sets the value of the base octave of a voice.
It is stored as 12 times the number of octaves.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   voice_baseoctave   */
/*----------------------*/
void voice_baseoctave(p, o)
    voice* p;
    int o;
    {
    if (!p)
        return;
    if (o < 0)
        return;
    p->baseoctave12 = o * 12;
    } /* End of function voice_baseoctave. */

/*---------------------------------------------------------------------------
voice_baseletter() sets the value of the base letter of a voice. It is
stored as (c - 'c') % 7.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   voice_baseletter   */
/*----------------------*/
void voice_baseletter(p, c)
    voice* p;
    register int c;
    {
    if (!p)
        return;
    if (c < 'a' || c > 'g')
        return;
    c -= 'c';
    if (c < 0)
        c += 7;
    p->baseletterC = c;
    } /* End of function voice_baseletter. */

/*---------------------------------------------------------------------------
voice_inversion() causes notes to be inverted before use, if the argument
is non-zero. This means that 'a' will be interpreted as 'A', and vice versa.
It is stored as 0x20, if set, to cause case inversion using xor.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    voice_inversion   */
/*----------------------*/
void voice_inversion(p, b)
    voice* p;
    int b;
    {
    if (!p)
        return;
    p->inversion = b ? 0x0020 : 0x0000;
    } /* End of function voice_inversion. */

/*---------------------------------------------------------------------------
voice_event_metro() adds a new metronome event to the event list for a voice.
The rat offered is copied!
---------------------------------------------------------------------------*/
/*----------------------*/
/*   voice_event_metro  */
/*----------------------*/
void voice_event_metro(p, time, speed)
    voice* p;
    rat* time;
    metrotype speed;
    {
    event* pe;
    metrotype* pm;

    if (!p || !time)
        return;
    pe = new_event();
    rat_assign_rat(&pe->time, time);
    pe->type = eMETRO;
    pm = (metrotype*)pe->params;    /* Trick to keep compiler happy. */
    *pm = speed;
    eventlist_addevent(&p->events, pe);
    } /* End of function voice_event_metro. */

/*---------------------------------------------------------------------------
voice_note() adds a note of given length to the eventlist for a voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      voice_note      */
/*----------------------*/
void voice_note(p, length, note, static_mode, staccato)
    voice* p;
    rat* length;
    int note;
    boolean static_mode;
    rat* staccato;
    {
    event* pe;
    rat* on_length = 0; /* Length of the on-part of the note. */

    if (!p || note < 0 || !length)
        return;
    if (staccato && rat_positive(staccato)) {
        on_length = new_rat();
        rat_assign_rat(on_length, length);
        rat_mult_rat(on_length, staccato);
        }
    note += p->compile_transpose;
    while (note < 0)
        note += 12;
    while (note > 0x7f)
        note -= 12;

    /* Schedule beginning of note: */
    pe = new_event();
    rat_assign_rat(&pe->time, &p->lasttime);
    pe->type = eON;
    pe->params[0] = note;
    pe->params[1] = p->on_velocity;
    eventlist_current_insert(&p->events, pe);

    /* Schedule end of note: */
    pe = new_event();
    rat_assign_rat(&pe->time, &p->lasttime);
    if (!on_length)
        rat_add_rat(&pe->time, length);
    else
        rat_add_rat(&pe->time, on_length);
    if (!static_mode)
        if (!on_length)
            rat_assign_rat(&p->lasttime, &pe->time);
        else
            rat_add_rat(&p->lasttime, length);
    pe->type = eOFF;
    pe->params[0] = note;
    pe->params[1] = p->off_velocity;
    eventlist_current_addevent(&p->events, pe);
    if (!static_mode)
        eventlist_current_time(&p->events, &p->lasttime);
    if (on_length)
        rat_delete(on_length);
    } /* End of function voice_note. */

/*---------------------------------------------------------------------------
voice_rest() adds a rest of given length to the eventlist for a voice.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      voice_rest      */
/*----------------------*/
void voice_rest(p, length)
    voice* p;
    rat* length;
    {
    if (!p || !length)
        return;

    /* Add the rest length to the "lasttime" of the event list: */
    rat_add_rat(&p->lasttime, length);
    eventlist_current_time(&p->events, &p->lasttime);
    } /* End of function voice_rest. */

/*----------------------*/
/*   voice_set_marker   */
/*----------------------*/
event* voice_set_marker(p, tag)
    voice* p;
    int tag;
    {
    event* pe;
    if (!p || tag < 0)
        return 0;

    pe = new_event();
    rat_assign_rat(&pe->time, &p->lasttime);
    pe->type = eMARKER;
    *(int*)pe->params = tag;
    eventlist_current_insert(&p->events, pe);
    return pe;
    } /* End of function voice_set_marker. */

/*---------------------------------------------------------------------------
voice_set_repeat(): The caller is expected to send the address of the first
marker of a span as a parameter. If it really is a marker, then a repeat-
event is inserted in the voice's event list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   voice_set_repeat   */
/*----------------------*/
event* voice_set_repeat(p, first)
    voice* p;
    event* first;
    {
    event* pe;
    if (!p || !first || first->type != eMARKER)
        return 0;

    pe = new_event();
    rat_assign_rat(&pe->time, &p->lasttime);
    pe->type = eREPEAT;
    *(event**)pe->params = first;
    eventlist_current_insert(&p->events, pe);
    return pe;
    } /* End of function voice_set_repeat. */

/*----------------------*/
/*   voice_setchannel   */
/*----------------------*/
void voice_setchannel(p, c)
    voice* p;
    int c;
    {
    if (!p || c < 1 || c > 16)
        return;
    p->channel = c - 1;
    } /* End of function voice_setchannel. */

/*--------------------------*/
/*  voice_set_on_velocity   */
/*--------------------------*/
void voice_set_on_velocity(p, c)
    voice* p;
    int c;
    {
    if (!p || c < 0x00 || c > 0x7f)
        return;
    p->on_velocity = c;
    } /* End of function voice_set_on_velocity. */

/*--------------------------*/
/*  voice_set_off_velocity  */
/*--------------------------*/
void voice_set_off_velocity(p, c)
    voice* p;
    int c;
    {
    if (!p || c < 0x00 || c > 0x7f)
        return;
    p->off_velocity = c;
    } /* End of function voice_set_off_velocity. */

/*----------------------*/
/*   voice_set_program  */
/*----------------------*/
void voice_set_program(p, i)
    voice* p;
    int i;
    {
    if (!p || i < 1)
        return;
    p->program = i - 1;
    } /* End of function voice_set_program. */

/*--------------------------*/
/*  voice_set_program_name  */
/*--------------------------*/
void voice_set_program_name(p, pc)
    voice* p;
    char* pc;
    {
    if (!p || !pc || !*pc)
        return;
    if (p->program_name)
        free(p->program_name);
    p->program_name = (char*)malloc(strlen(pc) + 1);
    strcpy(p->program_name, pc);
    } /* End of function voice_set_program_name. */

/*----------------------*/
/*    voice_set_name    */
/*----------------------*/
void voice_set_name(p, pc)
    voice* p;
    char* pc;
    {
    if (!p || !pc || !*pc)
        return;
    if (p->name)
        free(p->name);
    p->name = (char*)malloc(strlen(pc) + 1);
    strcpy(p->name, pc);
    } /* End of function voice_set_name. */

/*----------------------*/
/*       voice_pan      */
/*----------------------*/
void voice_pan(p, i)
    voice* p;
    int i;
    {
    if (!p)
        return;
    if (i < -7)
        i = -7;
    if (i > 7)
        i = 7;
    p->pan = (i + 8) << 3;
    p->pan_set = true;
    } /* End of function voice_pan. */

/*---------------------------------------------------------------------------
voice_perform_transpose() sets the absolute value of the performance-time
transposition for the whole voice. No checks are made for absurdity of the
transposition, since the note value is masked with 0x7f at performance time.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  voice_perform_transpose */
/*--------------------------*/
void voice_perform_transpose(p, i)
    voice* p;
    int i;
    {
    if (!p)
        return;
    p->perform_transpose = i;
    } /* End of function voice_perform_transpose. */

/*---------------------------------------------------------------------------
voice_compile_transpose() sets the absolute value of the compile-time
transposition for the voice. No checks are made for absurdity of the
transposition.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  voice_compile_transpose */
/*--------------------------*/
void voice_compile_transpose(p, i)
    voice* p;
    int i;
    {
    if (!p)
        return;
    p->compile_transpose = i;
    } /* End of function voice_compile_transpose. */

/*---------------------------------------------------------------------------
voice_event_transpose() inserts an event for the absolute value of the
perform-time transposition for the voice.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*   voice_event_transpose  */
/*--------------------------*/
event* voice_event_transpose(p, transpose)
    voice* p;
    int transpose;
    {
    event* pe;
    if (!p)
        return 0;
    pe = new_event();
    rat_assign_rat(&pe->time, &p->lasttime);
    pe->type = eTRANSPOSE;
    *(int*)pe->params = transpose;
    eventlist_current_insert(&p->events, pe);
    return pe;
    } /* End of function voice_event_transpose. */

/*---------------------------------------------------------------------------
voice_max_volume() sets the maximum volume for a voice. This value just
modulates the values in the volume events. The value 0 turns
off any kind of control by the volume events. The permitted values are from
0 to 127.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*     voice_max_volume     */
/*--------------------------*/
void voice_max_volume(p, volume)
    voice* p;
    int volume;
    {
    if (!p || volume < 0 || volume > 127)
        return;
    p->max_volume = volume;
    } /* End of function voice_max_volume. */

/*---------------------------------------------------------------------------
voice_event_volume() inserts an event for the absolute value of the
perform-time transposition for the voice. The scale is from 0 to 127.
The value is multiplied by voice::max_volume and divided by 127 at
performance time.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*    voice_event_volume    */
/*--------------------------*/
void voice_event_volume(p, volume)
    voice* p;
    int volume;
    {
    event* pe;
    if (!p || volume < 0 || volume > 127)
        return;
    pe = new_event();
    rat_assign_rat(&pe->time, &p->lasttime);
    pe->type = eVOLUME;
    pe->params[0] = volume;
    eventlist_current_insert(&p->events, pe);
    } /* End of function voice_event_volume. */

/*---------------------------------------------------------------------------
voice_event_note_value() inserts an event for the absolute value of the
perform-time unit note value for the voice. The scale is from 0 to 127.
The value is multiplied by song::note_value and 192*8 at performance time.
It's best to keep all of these events in voice 0. In fact, this should be
compulsory. Otherwise wierd effects will happen.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*  voice_event_note_value  */
/*--------------------------*/
void voice_event_note_value(p, time, value)
    voice* p;
    rat* time;
    rat* value;
    {
    event* pe;
    if (!p || !time || !value || !rat_positive(value))
        return;
    if (sizeof(rat) > 8) {
        printf("Warning: size of rat > 8.\n");
        return;
        }
    pe = new_event();
    rat_assign_rat(&pe->time, time);
    pe->type = eNOTE_VALUE;
    rat_assign_rat((rat*)pe->params, value);
    eventlist_addevent(&p->events, pe);
    } /* End of function voice_event_note_value. */

/*---------------------------------------------------------------------------
voice_event_program() inserts an event for a MIDI program change.
The value is from 1 to 128.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*   voice_event_program    */
/*--------------------------*/
void voice_event_program(p, program)
    voice* p;
    int program;
    {
    event* pe;
    if (!p || program < 1 || program > 128)
        return;
    pe = new_event();
    rat_assign_rat(&pe->time, &p->lasttime);
    pe->type = ePROGRAM;
    pe->params[0] = program - 1;
    eventlist_current_insert(&p->events, pe);
    } /* End of function voice_event_program. */

/*---------------------------------------------------------------------------
voices_dtor() destroys a certain kind of list of voices.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      voices_dtor     */
/*----------------------*/
void voices_dtor(p)
    voice** p;
    {
    voice *pv1, *pv2;
    if (!p)
        return;
    for (pv1 = *p; pv1; ) {
        pv2 = pv1->next;
        voice_delete(pv1);
        pv1 = pv2;
        }
    } /* End of function voices_dtor. */

/*---------------------------------------------------------------------------
voices_delete() deletes a "voices" list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     voices_delete    */
/*----------------------*/
void voices_delete(p)
    voice** p;
    {
    if (!p)
        return;
    voices_dtor(p);
    *p = 0;
    } /* End of function voices_delete. */

/*---------------------------------------------------------------------------
voices_findvoice() tries to find a voice with a given index.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   voices_findvoice   */
/*----------------------*/
voice* voices_findvoice(voices, v)
    voice** voices;
    int v;
    {
    voice* p;

    if (!voices)
        return 0;
    for (p = *voices; p; p = p->next)
        if (p->v == v)
            break;
    return p;
    } /* End of function voices_findvoice. */

/*---------------------------------------------------------------------------
voices_insertvoice() inserts a given voice *p into the list *voices, just
after the element *q of the list. If q == 0, then *p is inserted at the
beginning of the list. No check is made of whether q really points to an
element of the list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*  voices_insertvoice  */
/*----------------------*/
static void voices_insertvoice(voices, q, p)
    voice** voices;
    voice *q, *p;
    {
    if (!voices || !p)
        return;
    if (!q) {
        p->next = *voices;
        *voices = p;
        return;
        }
    p->next = q->next;
    q->next = p;
    } /* End of function voices_insertvoice. */

/*---------------------------------------------------------------------------
voices_newvoice() creates a new "voice" with a given index, and adds it to
the given list. If anything goes wrong, null is returned. A voice may not
have a negative index. Nor may two voices have the same index.
Voices are inserted in the list in increasing order of index.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    voices_newvoice   */
/*----------------------*/
voice* voices_newvoice(voices, v)
    voice** voices;
    int v;
    {
    voice *p, *q;

    if (!voices || v < 0)
        return 0;
    for (p = *voices, q = 0; p; p = p->next) {
        if (p->v >= v)
            break;
        q = p;
        }
    if (p && p->v == v) /* No two voices may have the same number. */
        return 0;
    p = new_voice();
    if (!p)
        return 0;
    p->v = v;
    voices_insertvoice(voices, q, p);

    return p;
    } /* End of function voices_newvoice. */

/*---------------------------------------------------------------------------
voices_getvoice() is like voices_findvoice(), but if the voice does not
exist, it is created. This is faster on average than voices_newvoice().
---------------------------------------------------------------------------*/
/*----------------------*/
/*    voices_getvoice   */
/*----------------------*/
static voice* voices_getvoice(p, v)
    voice** p;
    int v;
    {
    voice* pv = voices_findvoice(p, v);
    if (pv)
        return pv;
    return voices_newvoice(p, v);
    } /* End of function voices_getvoice. */

/*---------------------------------------------------------------------------
voices_set_voice_name() sets the MIDI-file name of a given voice.
If the voice does not exist, it is created.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*   voices_set_voice_name  */
/*--------------------------*/
void voices_set_voice_name(p, n, pc)
    voice** p;
    long n;
    char* pc;
    {
    voice* pv;

    if (!p || n < 0 || !pc || !*pc)
        return;
    pv = voices_getvoice(p, n);
    voice_set_name(pv, pc);
    } /* End of function voices_set_voice_name. */

/*---------------------------------------------------------------------------
voices_newmetro() adds a new event for voice 0, to show a metronome setting.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    voices_newmetro   */
/*----------------------*/
void voices_newmetro(p, time, speed)
    voice** p;
    rat* time;
    metrotype speed;
    {
    voice* pv;

    if (!p || !time)
        return;
    pv = voices_getvoice(p, 0);
    voice_event_metro(pv, time, speed);
    } /* End of function voices_newmetro. */

/*---------------------------------------------------------------------------
voices_new_note_value() adds a new event for voice 0, to set the note value.
---------------------------------------------------------------------------*/
/*--------------------------*/
/*   voices_new_note_value  */
/*--------------------------*/
void voices_new_note_value(p, time, value)
    voice** p;
    rat* time;
    rat* value;
    {
    voice* pv;

    if (!p || !time || !value)
        return;
    pv = voices_getvoice(p, 0);
    voice_event_note_value(pv, time, value);
    } /* End of function voices_new_note_value. */

/*---------------------------------------------------------------------------
voices_showvoices() prints descriptions of the set of voices to stdout.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   voices_showvoices  */
/*----------------------*/
void voices_showvoices(voices)
    voice** voices;
    {
    register voice* p;

    if (!voices)
        return;
    if (!*voices) {
        printf("The current list of voices is empty.\n");
        return;
        }
    printf("Current list of voices:\n");
    for (p = *voices; p; p = p->next) {
        printf("Voice %d: channel: %d, last time: ", p->v, p->channel + 1);
        rat_fprint(&p->lasttime, stdout);
        if (p->name)
            printf(" <%s>", p->name);
        if (p->pan_set)
            printf(", pan: %d", ((p->pan >> 3) - 8));
        if (p->program >= 0)
            printf(", program: %d", p->program + 1);
        if (p->program_name)
            printf(", program name: \"%s\"", p->program_name);
        if (p->perform_transpose)
            printf(", transpose: %d", p->perform_transpose);
        printf(".\n");
/*      printf("Address: 0x%lx.\n", p); /* For trace only. */
        }
    } /* End of function voices_showvoices. */
