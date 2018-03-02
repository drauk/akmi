/* src/akmi/interp.c   2013-5-18   Alan U. Kennington. */
/* $Id$ */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan U. Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Structs in this file:

stringkey
alter
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Functions in this file:

stringkey_ctor
new_stringkey
stringkey_dtor
alter_ctor
new_alter
alter_delete
pause
sendbyte
ratinterp
songinterp
interp
---------------------------------------------------------------------------*/

#include "interp.h"
#include "help.h"
#include "nonstdio.h"

#define commentchar   '%'
#define scoremodechar '$'
#define ratmodechar   '@'
#define commandchar   '!'

typedef struct STRINGKEY {
    struct STRINGKEY* next;
    char* s;                    /* The string. */
    int i;                      /* The key. */
    } stringkey;

typedef struct ALTER {          /* For the speed alteration stack. */
    struct ALTER* next;
    rat* net_rat;
    } alter;

/* Global variables!! But technically local to this file. */
static song* songs = 0;         /* The list of all songs. */
static song* currentsong = 0;   /* The current song. */
static int basicchannel = 0;    /* Basic channel for transmission to kbd. */

static stringkey* midi_programs;/* The list of MIDI programs. */
static alter* free_alters = 0;  /* The list of free used alter-structures. */

/*---------------------------------------------------------------------------
stringkey_ctor() constructs a string-key.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    stringkey_ctor    */
/*----------------------*/
void stringkey_ctor(p)
    stringkey* p;
    {
    if (!p)
        return;
    p->next = 0;
    p->s = 0;
    p->i = 0;
    } /* End of function stringkey_ctor. */

/*---------------------------------------------------------------------------
new_stringkey() returns a new stringkey structure.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     new_stringkey    */
/*----------------------*/
stringkey* new_stringkey()
    {
    stringkey* p;
    p = (stringkey*)malloc(sizeof(stringkey));
    stringkey_ctor(p);
    return p;
    } /* End of function new_stringkey. */

/*---------------------------------------------------------------------------
stringkey_dtor() deletes a stringkey structure.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    stringkey_dtor    */
/*----------------------*/
void stringkey_dtor(p)
    stringkey* p;
    {
    if (!p)
        return;
    if (p->s)
        free(p->s);
    free(p);
    } /* End of function stringkey_dtor. */

/*---------------------------------------------------------------------------
alter_ctor() constructs an alteration structure.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      alter_ctor      */
/*----------------------*/
void alter_ctor(p)
    alter* p;
    {
    if (!p)
        return;
    p->next = 0;
    p->net_rat = 0;
    } /* End of function alter_ctor. */

/*---------------------------------------------------------------------------
new_alter() returns a new alteration structure. This structure is taken
from the stack free_alters if it is non-empty.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       new_alter      */
/*----------------------*/
alter* new_alter()
    {
    register alter* p;
    if (free_alters) {
        p = free_alters;
        free_alters = p->next;
        }
    else
        p = (alter*)malloc(sizeof(alter));
    if (p)
        alter_ctor(p);
    return p;
    } /* End of function new_alter. */

/*---------------------------------------------------------------------------
alter_delete() deletes alteration structure. The memory allocated by the
net rat is freed. But the alter-structure is put into the list free_alters.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     alter_delete     */
/*----------------------*/
void alter_delete(p)
    alter* p;
    {
    if (!p)
        return;
    rat_delete(p->net_rat);
/*  free(p); */
    p->next = free_alters;
    free_alters = p;
    } /* End of function alter_delete. */

/*---------------------------------------------------------------------------
pause() is a very crude way of getting a pause of roughly  n  milliseconds.
---------------------------------------------------------------------------*/
/*----------------------*/
/*         pause        */
/*----------------------*/
static void pause(n)
    int n;
    {
    long i, m, s = 0, c = 50;
    if (n < 0)
        n = -n;
    m = (long)n;
    m *= 157; m /= 10;
    for (i = 0; i < m; ++i) {
        s += n % ((int)c + 1);
        s -= n % ((int)c + 1);
        }
    } /* End of function pause. */

/*---------------------------------------------------------------------------
sendbyte() sends a byte to the MIDI interface, if the parameter is
non-negative.
If the parameter of sendbyte is negative, it has the following meaning:
    sSEND       the string so far is sent
    sSTART      a recording is started
    sOFF        the recording is stopped
    sCONTINUE   the recording is continued
    sPLAYBACK   the current recording is sent
If the parameter is non-negative, then it is a byte to eventually send.
This routine was written for experimental purposes only.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       sendbyte       */
/*----------------------*/
static void sendbyte(b)
    int b;
    {
    static char mob[MIDIBUFSZ]; /* The string to be sent. */
    static int moblength = 0;

    static char mobrec[10*MIDIBUFSZ];   /* For making recordings. */
    static int mobreclength = 0;        /* Current length of recording. */
    static boolean recording = false;   /* True if currently recording. */

    if (b < 0) {
        if (b == sSEND) {
            if (moblength > 0) {
                Midiws(moblength, mob);
                moblength = 0;
                }
            }
        if (b == sSTART) {
            mobreclength = 0;
            recording = true;
            }
        if (b == sOFF)
            recording = false;
        if (b == sCONTINUE)
            recording = true;
        if (b == sPLAYBACK) {
            if (mobreclength > 0)
                Midiws(mobreclength, mobrec);
            }
        }
    else {
        b &= 0xff;
        mob[moblength++] = b;
        if (recording)
            mobrec[mobreclength++] = b;
        }
    } /* End of function sendbyte. */

/*---------------------------------------------------------------------------
ratinterp() is a function for interpreting rational number arithmetic
commands from a given file.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      ratinterp       */
/*----------------------*/
static void ratinterp(f)
    FILE* f;
    {
    boolean quitting = false;
#define MAXRATS 10
    static rat rats[MAXRATS];
    static boolean firstratcall = true;
    rat* rat1 = 0;
    int c0, c1, c2;
    int i, i1, i2, i3, i4;

    if (!f)
        return;
    if (firstratcall) {
        for (i = 0; i < MAXRATS; ++i)
            rat_ctor(&rats[i]);
        firstratcall = false;
        }

    for (quitting = false; !quitting; ) { /* Main command loop. */
        if (f == stdin)
            printf("Rat command: ");
        while ((c0 = inch(f)) != EOF)
            if (c0 != '\n')
                break;
        if (c0 == EOF)
            break;

        /* Special-character commands: */
        if (c0 == commentchar) {
            readln(f); /* Gobble rest of line. */
            continue;
            }
        if (c0 == ratmodechar) /* End of rat-mode. */
            break;
        if (c0 == '?') {
            help(stdout, helpRAT);
            continue;
            }

        switch(c0) {
        case 'a':   /* Add a rat to a rat. */
            fscanf(f, "%d", &i1);
            if (i1 < 0 || i1 >= MAXRATS) {
                printf("Rats are numbered from 0 to %d.\n", MAXRATS - 1);
                printf("Ignoring rest of line...\n");
                readln(f);
                break;
                }
            rat1 = get_rat(f);
            rat_add_rat(&rats[i1], rat1);
            rat_delete(rat1);
            rat_array_fprint(rats, MAXRATS, stdout);
            break;
        case 'H': /* Show a given help page. */
            fscanf(f, "%d", &i1);
            help(stdout, i1);
            break;
        case 'h': /* Show help page for rat mode. */
            help(stdout, helpRAT);
            break;
        case 'p':   /* Show a rat. */
            fscanf(f, "%d", &i1);
            if (i1 >= MAXRATS) {
                printf("Rats are numbered from 0 to %d.\n", MAXRATS - 1);
                break;
                }
            if (i1 < 0) {
                rat_array_fprint(rats, MAXRATS, stdout);
                break;
                }
            printf("Rat %d = ", i1);
            rat_fprint(&rats[i1], stdout);
            printf(".\n");
            break;
        case 'Q': /* Quit rat mode. */
        case 'q':
            quitting = true;
            printf("Quitting rat mode. Returning to basic mode...\n");
            break;
        case 's':   /* Assign to a rational number. */
            fscanf(f, "%d", &i1);
            if (i1 < 0 || i1 >= MAXRATS) {
                printf("Rats are numbered from 0 to %d.\n", MAXRATS - 1);
                printf("Ignoring rest of line...\n");
                readln(f);
                break;
                }
            rat1 = get_rat(f);
            rat_assign_rat(&rats[i1], rat1);
            rat_delete(rat1);
            rat_array_fprint(rats, MAXRATS, stdout);
            break;
        default: /* Unrecognised command. */
            printf("Unrecognised command: %c.\n", c0);
            if (f == stdin)
                help(stdout, helpRAT);
            readln(f);
            break;
            }
        }
    } /* End of function ratinterp. */

/*---------------------------------------------------------------------------
songinterp() interprets "song" commands from a given file.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      songinterp      */
/*----------------------*/
void songinterp(f)
    FILE* f;
    {
    boolean quitting = false;
    int c0, c1, c2;
    int i, i1, i2, i3, i4;
    unsigned int ui1;
    long l1;
    double metro = 0;       /* Multiple-use variable. */
    rat* rat1 = 0;
    song* ps = 0;
    char *pc = 0, *pc2 = 0;
    char buf[LINEBUFSIZE]; /* For names of files. */
    FILE* f2 = 0;
    int currentsignature = 0; /* C major. */
    accidental acc;
    alter* alter_stack = 0; /* Stack of rate alterations. */
    alter* next_alter;      /* The next alteration. */
    boolean static_mode = false; /* True in static note mode. */
    stringkey* pski;

    if (!f)
        return;
    for (quitting = false; !quitting; ) { /* Main command loop. */
        if (f == stdin)
            printf("Score command: ");
        while ((c0 = inch(f)) != EOF)
            if (c0 != '\n')
                break;
        if (c0 == EOF)
            break;

        /* Special-character commands: */
        if (c0 == commentchar) {
            readln(f); /* Gobble rest of line. */
            continue;
            }
        if (c0 == scoremodechar) /* End of song-mode. */
            break;
        if (c0 == '?') {
            help(stdout, helpSCORE);
            continue;
            }
        switch(c0) {
        /* Scheduling of notes, for the current voice: */
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
            c1 = inch(f);
            if (c1 == EOF)
                break;
            switch(c1) { /* Check out the accidental: */
            case '#':
                acc = accSHARP;
                break;
            case 'x':
                acc = acc2SHARP;
                break;
            case '*':
                acc = accFLAT;
                c2 = inch(f);
                if (c2 == '*')
                    acc = acc2FLAT;
                else
                    ungetc((char)c2, f);
                break;
            case 'n':
                acc = accNATURAL;
                break;
            default:
                ungetc((char)c1, f);
                acc = accNONE;
                break;
                } /* End of switch(c1). */
            rat1 = get_rat(f); /* Always returns a new rat. */
            if (alter_stack)
                rat_mult_rat(rat1, alter_stack->net_rat);
            song_note(currentsong, rat1, c0, (int)acc, static_mode);
            rat_delete(rat1);
            break;
        case 'H': /* Show a given help page. */
            fscanf(f, "%d", &i1);
            help(stdout, i1);
            break;
        case 'h': /* Show help page for score mode. */
            help(stdout, helpSCORE);
            break;
        case 'i': /* Read and interpret a file. */
            fscanf(f, LINEBUFSIZESTR, buf);
            /* Clobber the \n or \r (for PC files on Unix machines): */
            for (pc2 = buf; *pc2; ++pc2)
                if (*pc2 == '\n' || *pc2 == '\r') {
                    *pc2 = 0;
                    break;
                    }
            printf("Interpreting midi file %s...\n", buf);
            f2 = fopen(buf, "r");
            if (!f2) {
                printf("Failed to open file %s.\n", buf);
                break;
                }
            interp(f2, 0);
            fclose(f2);
            f2 = 0;
            break;
        case 'I':
            mdaemon_init();
            break;
        case 'j':   /* Insert a volume event in the current voice. */
            fscanf(f, "%d", &i1);
            song_event_volume(currentsong, i1);
            break;
        case 'J':   /* Set the maximum volume for the current voice. */
            fscanf(f, "%d", &i1);
            song_max_volume(currentsong, i1);
            break;
        case 'k':   /* Set the key signature. */
            fscanf(f, "%d", &i1);
            song_keysig(currentsong, i1);
            break;
        case 'K':   /* Kill a score - delete its memory. */
            fscanf(f, "%ld", &l1);
            if (currentsong && currentsong->number == l1)
                currentsong = 0;
            if (songs_delremove(&songs, l1) < 0)
                printf("Failed to delete score %ld.\n", l1);
            break;
        case 'l':   /* Set the current off-velocity of the current voice. */
            fscanf(f, "%d", &i1);
            song_voice_off_velocity(currentsong, i1);
            break;
        case 'L':   /* Set the current on-velocity of the current voice. */
            fscanf(f, "%d", &i1);
            song_voice_on_velocity(currentsong, i1);
            break;
        case 'm':   /* Set the metronome rate of a particular note size. */
            rat1 = get_rat(f);
            i1 = inch(f);
            if (i1 != '=')          /* Gobble an optional '='. */
                ungetc((char)i1, f);
            fscanf(f, "%lf", &metro);
            metro *= rat_double(rat1);
            if (metro > 0)
                song_metronome(currentsong, 60/metro);
            rat_delete(rat1);
            rat1 = 0;
            break;
        case 'M':   /* Set the metronome rate. */
            fscanf(f, "%lf", &metro);
            if (metro > 0)
                song_metronome(currentsong, 60/metro);
            break;
        case 'n':   /* Set the name of a MIDI program. */
            /* Note: This function should be done by querying the kbd. */
            fscanf(f, "%d", &i1); /* The number of the program. */
            pc = 0;     /* Read a string of the form "...": */
            c1 = inch(f);
            if (c1 == EOF)
                break;
            if (c1 != '\"')
                ungetc((char)c1, f);
            else {
                pc2 = buf;
                while ((c1 = getc(f)) != EOF && c1 != '\"' && c1 != '\n')
                    *pc2++ = c1;
                *pc2 = 0;
                if (c1 == '\n')
                    printf("Warning: String in 'n' command ended at EOL.\n");
                pc = buf;
                }
            if (!pc) { /* Undefining a program number. */
                /* ... */
                }
            else {
                for (pski = midi_programs; pski; pski = pski->next)
                    if (pski->s && strcmp(buf, pski->s) == 0)
                        break;
                if (pski) {
                    if (pski->s) /* This is always true! */
                        free(pski->s);
                    }
                else {
                    pski = new_stringkey();
                    pski->next = midi_programs;
                    midi_programs = pski;
                    }
                pski->s = (char*)malloc((int)(strlen(buf) + 1));
                strcpy(pski->s, buf);
                pski->i = i1;
                }
            break;
        case 'N':   /* Name a voice. Arguments i and the rest of the line. */
            l1 = 0;
            fscanf(f, "%ld", &l1);

            /* Get the name of the voice (the rest of the line): */
            /* String is terminated with '\n'. */
            pc = fgets(buf, LINEBUFSIZE, f);
            if (pc) { /* Skip leading white space: */
                while (isspace(*pc))
                    ++pc;
                /* Clobber the \n or \r (for PC files on Unix machines): */
                for (pc2 = pc; *pc2; ++pc2)
                    if (*pc2 == '\n' || *pc2 == '\r') {
                        *pc2 = 0;
                        break;
                        }
                }
            if (l1 < 0) {
                printf("\nError: Negative parameter for N command.\n");
                break;
                }
            song_set_voice_name(currentsong, l1, pc);
            break;
        case 'o':
            c1 = inch(f);
            if (c1 == EOF)
                break;
            c1 |= 0x20;
            if (c1 < 'a' || c1 > 'g') {
                printf("Bad letter chosen for octave base letter: %c\n", c1);
                break;
                }
            fscanf(f, "%d", &i1);
            song_base(currentsong, c1, i1);
            break;
        case 'O':   /* Set the base of the default octave. */
            printf("Warning: Obsolete score command 'O' detected!!!\n");
            fscanf(f, "%d", &i1);
            if (i1 < 0)
                break;
            song_basenote(currentsong, i1);
            break;
        case 'p':   /* Set the pan position of the current voice. */
            fscanf(f, "%d", &i1);
            song_voice_pan(currentsong, i1);
            break;
        case 'P':   /* Set the MIDI program of the current voice. */
            pc = 0;     /* Read a string of the form "...": */
            c1 = inch(f);
            if (c1 == EOF)
                break;
            if (c1 != '\"')
                ungetc((char)c1, f);
            else {
                pc2 = buf;
                while ((c1 = getc(f)) != EOF && c1 != '\"' && c1 != '\n')
                    *pc2++ = c1;
                *pc2 = 0;
                if (c1 == '\n')
                    printf("Warning: String in 'P' command ended at EOL.\n");
                pc = buf;
                }
            if (!pc) { /* Setting to explicit integer. */
                fscanf(f, "%d", &i1);
                song_set_program(currentsong, i1);
                break;
                }
            for (pski = midi_programs; pski; pski = pski->next)
                if (pski->s && strcmp(buf, pski->s) == 0)
                    break;
            if (pski) {
                song_set_program(currentsong, pski->i);
                song_set_program_name(currentsong, pski->s);
                }
            else
                printf("MIDI program \"%s\" not defined.\n", buf);
            break;
        case 'Q': /* Quit score mode. */
        case 'q':
            quitting = true;
            printf("Quitting score mode. Returning to basic mode...\n");
            break;
        case 'r':   /* Rest. */
            rat1 = get_rat(f);
            if (alter_stack)
                rat_mult_rat(rat1, alter_stack->net_rat);
            song_rest(currentsong, rat1);
            rat_delete(rat1);
            break;
        case 'R':   /* Send pause via music daemon. */
            /* Only works if a score is currently being peformed! */
            fscanf(f, "%ld", &l1); /* Length of rest in mS. */
            l1 &= RELTIMEMASK;
            mdaemon_senddelay_wait(l1);
            break;
        case 's':   /* Set the current score to the given number. */
            fscanf(f, "%ld", &l1);
            if (l1 < 0) {
                if (!currentsong) {
                    printf("There is no current score.\n");
                    break;
                    }
                printf("The current score is:\n%u\t%s\n",
                    currentsong->number,
                    currentsong->title ? currentsong->title : "No title");
                break;
                }
            ps = songs_find(&songs, l1);
            if (!ps) {
                printf("Score %ld does not seem to exist yet.\n", l1);
                if (f == stdin)
                    readln(f);
                else
                    quitting = true;
                break;
                }
            currentsong = ps;
            break;
        case 'S':   /* Start a new score. */
            fscanf(f, "%ld", &l1);
            if (l1 < 0) {
                songs_showsongs(&songs);
                if (!currentsong)
                    printf("\nThere is no current score.\n");
                else
                    printf("\nThe current score is:\n%u\t%s\n\n",
                      currentsong->number,
                      currentsong->title ? currentsong->title : "No title");
                break;
                }
            ps = songs_newsong(&songs, l1);
            if (!ps) {
                printf("Something went wrong while creating score %u.\n", l1);
                if (f == stdin)
                    readln(f);
                else
                    quitting = true;
                break;
                }
            currentsong = ps;

            /* Get the title of the score (the rest of the line): */
            /* String is terminated with '\n'. */
            pc = fgets(buf, LINEBUFSIZE, f);
            if (pc) { /* Skip leading white space: */
                while (isspace(*pc))
                    ++pc;
                /* Clobber the \n or \r (for PC files on Unix machines): */
                for (pc2 = pc; *pc2; ++pc2)
                    if (*pc2 == '\n' || *pc2 == '\r') {
                        *pc2 = 0;
                        break;
                        }
                }
            song_newtitle(currentsong, pc);
            break;
        case 't':   /* Set current transposition offset for current voice. */
            fscanf(f, "%d", &i1);
            song_compile_transpose(currentsong, i1);
            break;
        case 'T':   /* Transpose the whole voice at performance time. */
            fscanf(f, "%d", &i1);
            song_perform_transpose(currentsong, i1);
            break;
        case 'u':   /* Insert absolute note value event in current voice.
                       Set a run-time absolute multiplier for
                       midi file note values.
                       u <rat> changes each unit note to have value <rat>.
                       For exact results, 192*4*<rat> should be an integer. */
            rat1 = get_rat(f);
            if (rat_positive(rat1))
                song_event_note_value(currentsong, rat1);
            else
                printf("Warning: u command parameter was not positive.\n");
            rat_delete(rat1);
            rat1 = 0;
            break;
        case 'U':   /* Set the performance-time relative multiplier for
                       midi file note values.
                       U <rat> means that each unit note has value <rat>.
                       For exact results, 192*4*<rat> should be an integer. */
            rat1 = get_rat(f);
            if (rat_positive(rat1))
                song_perform_note_value(currentsong, rat1);
            else
                printf("Warning: U command parameter was not positive.\n");
            rat_delete(rat1);
            rat1 = 0;
            break;
        case 'v':   /* Change current voice. */
            fscanf(f, "%d", &i1);
            if (i1 < 0) {
                printf("The current voice is %d.\n",
                            song_currentvoice(currentsong, -1));
                break;
                }
            if (!currentsong)
              printf("Attempt to set current voice when score is undefined.\n");
            else if (!song_findvoice(currentsong, i1)) {
/*                printf("Creating new voice %d on the fly...\n", i1); */
                song_newvoice(currentsong, i1);
                song_currentvoice(currentsong, i1);
                }
            else
                song_currentvoice(currentsong, i1);
            break;
        case 'V':   /* Create a new voice. */
            fscanf(f, "%d", &i1);
            if (i1 < 0) {
                song_showvoices(currentsong);
                break;
                }
            song_newvoice(currentsong, i1);
            song_currentvoice(currentsong, i1);
            break;
        case 'w':   /* Set the channel for the current voice. */
            fscanf(f, "%d", &i1);
            if (i1 < 1 || i1 > 16) {
                printf("The channel must be in the range 1 to 16.\n");
                break;
                }
            song_setchannel(currentsong, i1);
            break;
        case 'W':   /* Insert transposition event for current voice. */
            fscanf(f, "%d", &i1);
            song_event_transpose(currentsong, i1);
            break;
        case 'x':   /* Display the events of a given voice. */
            fscanf(f, "%d", &i1);
            song_showevents(currentsong, i1);
/*          song_showallevents(currentsong); */
            break;
        case 'X':   /* Set the staccato factor of the current song. */
            rat1 = get_rat(f);
            /* Should define song_set_staccato() to do the following: */
            if (!currentsong)
                break;
            if (rat_equals_long(rat1, (long)1) || !rat_positive(rat1)) {
                rat_delete(rat1);
                rat1 = 0;
                }
            if (currentsong->staccato_rat)
                rat_delete(currentsong->staccato_rat);
            currentsong->staccato_rat = rat1;
            break;
        case 'y':   /* Relative speed multiplier. */
            fscanf(f, "%lf", &metro);
            song_speed_multiply(currentsong, metro);
            break;
        case 'Y':   /* Absolute speed multiplier. */
            fscanf(f, "%lf", &metro);
            song_speed_multiplier(currentsong, metro);
            break;
        case 'z':   /* Perform a score. Syntax "z <n>" or "z ( <r> ) <n>". */
            c1 = inch(f);
            if (c1 != '(') {
                ungetc((char)c1, f);
                rat1 = new_rat();
                }
            else {
                rat1 = get_rat(f);
                while (c1 != EOF && c1 != ')')
                    c1 = inch(f);
                }
            fscanf(f, "%d", &i1);
            ps = songs_find(&songs, i1);
            song_play(ps, rat1);
            rat_delete(rat1);
            break;
        case 'Z':   /* Create midi file. Syntax "Z <n>" or "z ( <r> ) <n>". */
            c1 = inch(f);
            if (c1 != '(') {
                ungetc((char)c1, f);
                rat1 = new_rat();
                }
            else {
                rat1 = get_rat(f);
                while (c1 != EOF && c1 != ')')
                    c1 = inch(f);
                }
            fscanf(f, "%d", &i1);
            if (i1 >= 0) {
                ps = songs_find(&songs, i1);
                song_file(ps, rat1);
                }
            else
                for (ps = songs; ps; ps = ps->next)
                    song_file(ps, rat1);
            rat_delete(rat1);
            break;
        case '(':   /* Syntax: "(" rat note note note ... ")". */
            next_alter = new_alter();
            next_alter->net_rat = get_rat(f);
            if (alter_stack)
                rat_mult_rat(next_alter->net_rat, alter_stack->net_rat);
            next_alter->next = alter_stack;
            alter_stack = next_alter;
            break;
        case ')':
            if (alter_stack) {
                next_alter = alter_stack->next;
                alter_delete(alter_stack);
                alter_stack = next_alter;
                }
            break;
        case '[': /* Enter static note mode. */
            static_mode = true;
            break;
        case ']': /* Exit static note mode. */
            static_mode = false;
            break;
        case '@': /* Span marker. */
            fscanf(f, "%d", &i1);
            song_set_marker(currentsong, i1);
            break;
        case '^': /* Repeat a span. */
            fscanf(f, "%d", &i1);
            song_set_repeat(currentsong, i1);
            break;
        case 'P' & 0x1f: /* Control-P: interrupt playing score. */
            mdaemon_term();
            break;
        case commandchar: { /* Command introducer. */
            /* Note: This re-use of a distant buffer could be dangerous. */
            int n = readname(f, buf, LINEBUFSIZE);
            if (n < 0) {
                printf("ERROR: while reading `!` name.\n");
                if (f == stdin)
                    help(stdout, helpSCORE);
                readln(f);
                break;
                }
            if (n <= 0) {
                printf("ERROR: no name following the `!` character.\n");
                if (f == stdin)
                    help(stdout, helpSCORE);
                readln(f);
                break;
                }
            if (strcmp(buf, "prog") == 0) {
                /* Insert an event for program change. */
                fscanf(f, "%d", &i1);
                song_event_program(currentsong, i1);
                }
            else {
                printf("ERROR: unrecognized name \"%s\" following `!`.\n",
                    buf);
                if (f == stdin) {
                    help(stdout, helpSCORE);
                    readln(f);
                    }
                break;
                }
            }
            break;
        default: /* Unrecognised command. */
            printf("Unrecognised command: %c.\n", c0);
            if (f == stdin)
                help(stdout, helpSCORE);
            readln(f);
            break;
            }
        }
    } /* End of function songinterp. */

/*---------------------------------------------------------------------------
interp() is the top-level interpreter for files containing MIDI commands.
---------------------------------------------------------------------------*/
/*----------------------*/
/*        interp        */
/*----------------------*/
void interp(f, mode)
    FILE* f;
    int mode;
    {
    boolean quitting = false;
    int c0, c1, c2;
    int i, i1, i2, i3, i4;
    long l;
    char *pc, *pc2;
    char bytebuf[20];       /* Buffer for bytes to send. */
    char buf[LINEBUFSIZE];  /* For names of files. */
    FILE* f2 = 0;

    if (!f)
        return;

    /* If mode == 1, go to score mode first. Then come back to basic mode: */
    if (mode == 1)
        songinterp(f);

    for (quitting = false; !quitting; ) { /* Main command loop. */
        if (f == stdin)
            printf("Basic command: ");
        while ((c0 = inch(f)) != EOF)
            if (c0 != '\n')
                break;
        if (c0 == EOF)
            break;

        /* Special-character commands: */
        if (c0 == commentchar) {
            readln(f); /* Gobble rest of line. */
            continue;
            }
        if (c0 == scoremodechar) {
            songinterp(f);
            continue;
            }
        if (c0 == ratmodechar) {
            ratinterp(f);
            continue;
            }
        if (c0 == '?') {
            help(stdout, helpBASIC);
            continue;
            }
        switch(c0) {
        case 'A':   /* Turn off all notes on all channels. */
            for (i1 = 0; i1 <= 15; ++i1)        /* For all channels... */
                for (i2 = 0; i2 <= 127; ++i2) { /* For all notes... */
                    sendbyte(0x80 | i1);
                    sendbyte(i2);
                    sendbyte(0x7f);
                    sendbyte(sSEND);
                    }
            break;
        case 'B':   /* Read the MIDI input buffer. Return on key press. */
            mdaemon_read_midi_buf(1);
            break;
        case 'b':   /* Read the MIDI input buffer. Return when empty. */
            mdaemon_read_midi_buf(0);
            break;
        case 'C':   /* Change basic channel. */
            fscanf(f, "%d", &i1);
            basicchannel = i1 & 0x0f;
            break;
        case 'c':   /* Continue recording. */
            sendbyte(sCONTINUE);
            break;
        case 'd':   /* Damper on, on the basic channel. */
            sendbyte(0xb0);
            sendbyte(0x40);
            sendbyte(0x7f);
            sendbyte(sSEND);
            break;
        case 'D':   /* Damper off, on the basic channel. */
            sendbyte(0xb0 | basicchannel);
            sendbyte(0x40);
            sendbyte(0);
            sendbyte(sSEND);
            break;
        case 'H': /* Show a given help page. */
            fscanf(f, "%d", &i1);
            help(stdout, i1);
            break;
        case 'h': /* Show help page for basic mode. */
            help(stdout, helpBASIC);
            break;
        case 'i': /* Read and interpret a file. */
            fscanf(f, LINEBUFSIZESTR, buf);
            /* Clobber the \n or \r (for PC files on Unix machines): */
            for (pc2 = buf; *pc2; ++pc2)
                if (*pc2 == '\n' || *pc2 == '\r') {
                    *pc2 = 0;
                    break;
                    }
            printf("Interpreting midi file %s...\n", buf);
            f2 = fopen(buf, "r");
            if (!f2) {
                printf("Failed to open file %s.\n", buf);
                break;
                }
            interp(f2, 0);
            fclose(f2);
            f2 = 0;
            break;
        case 'K':   /* Stop the music daemon clock. */
            mdaemon_clock_stop();
            break;
        case 'k':   /* Start the music daemon clock. */
            mdaemon_clock_start();
            break;
        case 'l':   /* Send note via music daemon. */
            fscanf(f, "%ld", &l); /* Number of note. */
            l &= RELTIMEMASK;
            mdaemon_senddelay_wait(l);
            break;
        case 'm': /* Display free memory in heap. */
/*            printf("Free memory = %ld.\n", (long)Malloc((long)-1L)); */
#ifndef linux
            mallocmap();
#endif
            /* Also should show number of free/used rats, events, alters. */
            break;
        case 'n': /* Sound a note on the basic channel. */
            fscanf(f, "%d", &i1);
            i1 &= 0x7f;
            sendbyte(0x90 | basicchannel);
            sendbyte(i1);
            sendbyte(0x7f);
            sendbyte(sSEND);
            break;
        case 'o': /* Turn a note off, on the basic channel. */
            fscanf(f, "%d", &i1);
            i1 &= 0x7f;
            sendbyte(0x80 | basicchannel);
            sendbyte(i1);
            sendbyte(0x40);
            sendbyte(sSEND);
            break;
        case 'P':   /* Change program on the basic channel. */
            fscanf(f, "%d", &i1);
            i1 &= 0x7f;
            sendbyte(0xc0 | basicchannel);
            sendbyte(i1);
            sendbyte(sSEND);
            break;
        case 'p':   /* Play back recording. */
            sendbyte(sPLAYBACK);
            break;
        case 'Q': /* Quit. */
        case 'q':
            quitting = true;
            printf("Quitting...\n");
            mdaemon_term();
            break;
        case 'R':   /* Stop recording. */
            sendbyte(sOFF);
            break;
        case 'r':   /* Start recording. */
            sendbyte(sSTART);
            break;
        case 'S':   /* Show the current state of the daemon buffer. */
            mdaemon_show();
            break;
        case 's':   /* Send note via music daemon. */
            fscanf(f, "%d", &i1); /* Number of note. */
            i1 &= 0x7f;
            bytebuf[0] = 0x90 | basicchannel;
            bytebuf[1] = i1;
            bytebuf[2] = 0x40;
            mdaemon_sendbytes_wait(3, bytebuf);
            break;
        case 'T':   /* Delay a given number of milliseconds. */
            fscanf(f, "%d", &i1); /* Period of delay. */
            pause(i1);
            break;
        case 't':   /* Display daemon time. */
            printf("Daemon time is %ld mS.\n", mdaemon_time());
            break;
        case 'W':   /* Stop the daemon. */
            mdaemon_term();
            break;
        case 'w':   /* Start the daemon. */
            mdaemon_init();
            break;
        case 'X': /* Byte to send to midi-out in hexadecimal. */
            fscanf(f, "%x", &i1);
            i2 = i1 & 0xff;
            if (i2 != i1) {
                printf("Warning: two byte integer %x received.\n", i1);
                printf("Using the lower byte, %x, instead.\n", i2);
                }
            sendbyte(i2);
            sendbyte(sSEND);
            break;
        case 'x': /* 3 bytes to send to midi-out in hexadecimal. */
            fscanf(f, "%x %x %x", &i1, &i2, &i3);
            sendbyte(i1 & 0xff);
            sendbyte(i2 & 0xff);
            sendbyte(i3 & 0xff);
            sendbyte(sSEND);
            break;
        case 'Y': /* Make a copy of all help pages to a file. */
            fscanf(f, LINEBUFSIZESTR, buf);
            /* Clobber the \n or \r (for PC files on Unix machines): */
            for (pc2 = buf; *pc2; ++pc2)
                if (*pc2 == '\n' || *pc2 == '\r') {
                    *pc2 = 0;
                    break;
                    }
            printf("Writing all help pages to file %s...\n", buf);
            f2 = fopen(buf, "w");
            if (!f2) {
                printf("Failed to open file %s.\n", buf);
                break;
                }
            help(f2, helpALL);
            fclose(f2);
            f2 = 0;
            break;
        default: /* Unrecognised command. */
            printf("Unrecognised command: %c.\n", c0);
            readln(f);
            if (f == stdin)
                help(stdout, helpSCORE);
            break;
            }
        }
    } /* End of function interp. */
