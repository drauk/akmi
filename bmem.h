/* src/akmi/bmem.h   24 June 1995   Alan Kennington. */
/* $Id$ */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_BMEM_H
#define AKMI_BMEM_H
/*---------------------------------------------------------------------------
This file defines a class of block memory allocators.

Structs defined in this file:

struct  bmem
---------------------------------------------------------------------------*/

typedef struct BMEM {
    char* free;         /* The first char of the first free block. */
    long usersize;
    long chunksize;
    long nchunks;
    long blocksize;
    } bmem;

extern void     bmem_ctor();
extern bmem*    new_bmem();
extern void     bmem_dtor();
extern void     bmem_delete();
extern char*    bmem_newchunk();
extern void     bmem_freechunk();

/* Objects to be initialised by main(): */
extern bmem eventmem;
extern bmem ratmem;

#endif /* AKMI_BMEM_H */
