/* src/akmi/help.c   26 January 1999   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

showpage
help
---------------------------------------------------------------------------*/

#include "help.h"
#include <stdio.h>

static char *helppageTOP[] = { /* Top menu page. */
    "AKMI program information.  Usage:  akmi {<filename>}",
    "",
    "The program has 3 modes: SCORE, BASIC and RAT.",
    "The program starts in score mode, but files start in basic mode.",
    "To go from score mode to basic mode, type $, q or Q.",
    "To quit, type q or Q in basic mode. In score mode, type qq to quit.",
    "",
    "Help pages:",
    "     0  top menu page (this page)",
    "     1  score mode summary, part 1 (basic commands)",
    "     2  score mode summary, part 2 (more basic commands)",
    "     3  score mode summary, part 3 (command spec syntax + rationals)",
    "     4  score mode summary, part 4 (time definitions)",
    "     5  basic mode summary",
    "     6  rat mode summary",
    "    -1  show all help pages",
    "To get the help page for the current mode, type ? or h.",
    "To see this help page again, type H0.",
    "The command prompt tells you which mode you are in.",
    "",
    (char*)0
    };
static char* helppageSCORE1[] = { /* Score mode help page. */
    "AKMI score mode, part 1 of 4:",
    "<n>{<x>}{<rat>} note <n> (a,A,b,B,...g,G), accidental <x>, length <rat>",
    "                    accidentals can be: #, x, *, **, or n",
    "H <i>           show help page i (-1 for all pages)",
    "h,?             show this help page",
    "i <filename>    take input from named file (initially in basic mode)",
    "k <i>           set key signature for reading current score to i sharps",
    "m <r> {=} <f>   set metronome to r = f, with r rational, optional \"=\"",
    "M <f>           set metronome rate to f (floating point)",
    "o <i>           set base note of current voice to i",
    "r {<rat>}       insert a rest of length <rat>",
    "S <i> <title>   create score with index i, and title = rest of line",
    "S -1            show all current score titles",
    "v <i>           commence score-line for voice i",
 "V <i>           create voice i (if i < 0 => show all voices) [cf command N]",
    "z {(<rat>)} <i> perform score i; optional starting point in parentheses",
 "Z {(<rat>)} <i> create midi file s<i>.mid for score i; i = -1 => all scores",
    "$,q,Q           exit score mode",
    "$q,$Q,qq,QQ     exit the program",
    (char*)0
    };
static char* helppageSCORE2[] = { /* Score mode extra help page. */
    "AKMI score mode, part 2 of 4:",
    "I               initialise the midi interface daemon [is this useful?]",
    "J/j <i>         set volume scale to i / insert event for rel. volume i",
    "K/s <i>         delete score i / change current score to i",
    "N <i> <title>   create voice i; MIDI-file name of voice = rest of line",
    "P/p/T <i>       set program/pan/transpose of current voice to i",
    "u <rat>         schedule event for absolute unit note value",
    "U <rat>         set perf. time relative unit note value for current score",
    "W <i>           insert transposition event of amount i",
    "x/t <i>         show event list of voice i / immediate transposition = i",
    "X <rat>         set the staccato factor to fraction <rat>; e.g. X/2abX",
    "y/Y <rat>       set relative/absolute speed-up of current score to <rat>",
    "( <rat>         lengthen all notes up to following ')' by factor <rat>",
    ")               terminate the lengthening of notes (can be nested)",
    "                    e.g. (/2a(2/3b)c/2) is equivalent to a/2b/3c/4.",
    "[               stop time from moving, so that a chord can be formed",
    "]               start time moving normally again; e.g. [ceg]C is a chord",
    "!prog <i>       insert program-change event for current voice to i",
    "%               ignore the rest of the line (i.e. commence comment)",
    "ctrl-P          interrupt playing of current score",
    (char*)0
    };
static char* helppageSCORE3[] = { /* Score mode extra help page. */
    "AKMI score mode, part 3 of 4:",
    "    In syntax rules, a comma ',' separates equivalent or near-equivalent",
    "    commands. A slash '/' separates two or more commands which are being",
    "    explained on the same line.",
    "",
    "    Braces '{' and '}' indicate optional arguments.",
    "    Angle brackets '<' and '>' indicate an argument, not a literal.",
    "",
    "    Rational numbers have the formats:",
    "        i, n/d, i + n/d, i - n/d, /d, or nothing.",
    "    The form \"/d\" is equivalent to \"1/d\".",
    "    If no value is given, the default value of 1 is used.",
    "",
    "    Note that all upper and lower case alphabet letters have now",
    "    been used as commands in score mode!",
    (char*)0
    };
static char* helppageSCORE4[] = { /* Score mode extra help page. */
    "AKMI score mode, part 4 of 4:",
    "",
    "    Time is a funny thing. Means different things to different robots.",
    "    In the new time definition, the metronome settings refer to MIDI-file",
    "    1/4-notes (!!!), not akmi time units as previously. This is a change!",
    "",
    "    Previously, note times in akmi files were abstract, and the metronome",
    "    settings (m/M commands) just controlled the unit note speeds.",
    "    Now the commands u/U give midi 1/4-note significance to all notes",
    "    in an akmi file. The command \"u/8\" means that all akmi notes",
    "    are written out as 1/8 notes in the midi file. But the metronome",
    "    setting refers to the midi 1/4 notes now, not the akmi unit notes.",
    "    So \"u/8 m = 120\" gives a speed of 240 akmi notes per minute.",
    "",
    "    If you do not use the u/U commands, then there is no difference!",
    "    But then all of the akmi notes will be written as 1/4 notes in",
    "    the midi file, which may be inconvenient in a score printout.",
    (char*)0
    };
static char *helppageBASIC[] = { /* Basic mode help page. */
    "AKMI basic mode:",
    "A/d/D         turn off all notes / turn damper on/off on basic channel",
    "b/B           show MIDI input buffer; return on empty buffer/key-press",
    "C <c>         change the basic channel to c (for all sending)",
    "H <i>         show help page i (-1 for all pages)",
    "h,?           show this help page",
    "i <filename>  take input from a file",
    "k/K/t         start/stop/show the music daemon clock",
    "l <i>         put a delay of length i pseudo-mS in the message buffer",
    "m/S           show free memory / show current state of message buffer",
    "n/o <i>       turn note on/off at pitch i on the basic channel",
    "P <p>         change program to p on the basic channel",
    "q,Q           quit",
    "r/R/c/p       start/end/continue/playback a \"recording\"",
    "s/T <i>       send note of pitch i on the basic channel / delay of i mS",
    "w/W           start/stop a music daemon",
    "x <i> <j> <k> send 3 hexadecimal bytes",
    "X <n>         send a single hexadecimal byte n",
    "Y <filename>  write all help pages to a file",
    "$/@/%         enter score mode / enter rat mode / ignore rest of line",
    (char*)0
    };
static char* helppageRAT[] = { /* Rat mode help page. */
    "AKMI rat mode:",
    "a <i> <r>       add r to rat i",
    "p <i>           show rat i (i < 0 to show all rats)",
    "s <i> <r>       set rat i to r",
    "@,q,Q           exit rat mode",
    "H <i>           show help page i (-1 for all pages)",
    "h,?             show this help page",
    "%               ignore the rest of the line",
    (char*)0
    };

char** helppages[] = {
    helppageTOP,
    helppageSCORE1,
    helppageSCORE2,
    helppageSCORE3,
    helppageSCORE4,
    helppageBASIC,
    helppageRAT,
    (char**)0
    };

/*---------------------------------------------------------------------------
showpage() copies a single page of help information to a given file.
The page of help information is expected to be a null-terminated array of
pointers to strings (like argv).
---------------------------------------------------------------------------*/
/*----------------------*/
/*       showpage       */
/*----------------------*/
static void showpage(f, page)
    FILE* f;
    char** page;
    {
    char** ppc;

    if (!f || !page)
        return;
    for (ppc = page; *ppc; ++ppc)
        fprintf(f, "%s\n", *ppc);
    } /* End of function showpage. */

/*---------------------------------------------------------------------------
help() shows a given page of help information, or all pages if asked for
a negative page, on a given file. The global array of arrays of strings,
"helppages" is used for the help pages.
---------------------------------------------------------------------------*/
/*----------------------*/
/*         help         */
/*----------------------*/
void help(f, i) /* Show a help page. */
    FILE* f;
    int i;
    {
    int npages;
    char ***pppc;

    if (!f)
        return;
    for (pppc = helppages, npages = 0; *pppc; ++pppc)
        ++npages;
    if (i >= npages)
        fprintf(f, "Help page %d does not exist!\n", i);
    else if (i >= 0) { /* A single page. */
        fprintf(f, "Help page %d:\n", i);
        showpage(f, helppages[i]);
        }
    else { /* All the pages. */
        for (i = 0; i < npages; ++i) {
            fprintf(f, "Help page %d:\n\n", i);
            showpage(f, helppages[i]);
            if (i < npages - 1)
                fprintf(f, "\n");
            }
        }
    } /* End of function help. */
