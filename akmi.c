/* src/akmi/akmi.c   27 June 1995   Alan Kennington. */
/* $Id$ */
/* Alan Kennington's Music Interpreter. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

main
---------------------------------------------------------------------------*/

#include "interp.h"
#include "help.h"
#include "bmem.h"

/*----------------------*/
/*         main         */
/*----------------------*/
int main(argc, argv)
    int argc;
    char** argv;
    {
    FILE* f;
/*  setbuf(stdout, (char*)0); */

    /* Initialise global objects. */
    bmem_ctor(&eventmem, (long)sizeof(event), (long)20);
    bmem_ctor(&ratmem, (long)sizeof(rat), (long)20);

    /* Relic from the first couple of days of programming: */
/*    printf("Testing the midi-out port:\n"); */
    help(stdout, helpTOP);
    if (argc == 2) {
        f = fopen(argv[1], "r");
        if (!f)
            printf("Failed to open file %s.\n", argv[1]);
        else {
            printf("Interpreting file %s...\n", argv[1]);
            interp(f, 0);
            }
        }
    interp(stdin, 1);
    return 0;
    } /* End of function main. */
