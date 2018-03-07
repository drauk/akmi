/* src/akmi/bmem.c   24 June 1995   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

bmem_ctor
new_bmem
bmem_dtor
bmem_delete
bmem_getnewblock
bmem_newchunk
bmem_freechunk

Note that functions bmem_... take bmem* as the first parameter.
---------------------------------------------------------------------------*/

#include "bmem.h"
#include "malloc.h"

/* #include <stdio.h> */

/*---------------------------------------------------------------------------
bmem_ctor() constructs a block memory allocator.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      bmem_ctor       */
/*----------------------*/
void bmem_ctor(p, s, n)
    bmem* p;
    long s;
    long n;
    {
    register char* q;
    register char* r;
    register long i;

    if (!p)
        return;
    p->usersize = s;
    p->chunksize = s + sizeof(char*);
    p->nchunks = n;
    p->blocksize = n * p->chunksize;

    /* The rest is equivalent to calling bmem_getnewblock: */
    p->free = (char*)malloc((int)p->blocksize);

    /* Initialise all chunks: */
    q = r = p->free;
    for (i = n; --i > 0; ) {
        r += p->chunksize;
        *(char**)q = r;
        q = r;
        }
    *(char**)q = 0;
    } /* End of function bmem_ctor. */

/*---------------------------------------------------------------------------
new_bmem() returns a new bmem structure.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       new_bmem       */
/*----------------------*/
bmem* new_bmem(n, s)
    long n;
    long s;
    {
    bmem* p;

    p = (bmem*)malloc((int)sizeof(bmem));
    if (p)
        bmem_ctor(p, n, s);
    return p;
    } /* End of function new_bmem. */

/*----------------------*/
/*      bmem_dtor       */
/*----------------------*/
void bmem_dtor(p)
    bmem* p;
    {
    /* Can't free all the blocks, because there is no block linkage field. */
    } /* End of function bmem_dtor. */

/*---------------------------------------------------------------------------
bmem_delete() deletes a bmem structure. Probably there are not very many
situations under which this function should really be called.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      bmem_delete     */
/*----------------------*/
void bmem_delete(p)
    bmem* p;
    {
    if (!p)
        return;
/*  bmem_dtor(p); */
    free(p);
    } /* End of function bmem_delete. */

/*---------------------------------------------------------------------------
bmem_getnewblock() gets a new block of memory when free == 0.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   bmem_getnewblock   */
/*----------------------*/
static void bmem_getnewblock(p)
    bmem* p;
    {
    register char* q;
    register char* r;
    register long i;

    if (!p)
        return;
    p->free = (char*)malloc((int)p->blocksize);

    /* Initialise all chunks: */
    q = r = p->free;
    for (i = p->nchunks; --i > 0; ) {
        r += p->chunksize;
        *(char**)q = r;
        q = r;
        }
    *(char**)q = 0;
    } /* End of function bmem_getnewblock. */

/*---------------------------------------------------------------------------
bmem_newchunk() returns a new chunk from the free list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     bmem_newchunk    */
/*----------------------*/
char* bmem_newchunk(p)
    bmem* p;
    {
    char* q;
    if (!p)
        return 0;
    if (!p->free)
        bmem_getnewblock(p);
    q = p->free;
    p->free = *(char**)q;
    return q + sizeof(char*);
    } /* End of function bmem_newchunk. */

/*---------------------------------------------------------------------------
bmem_freechunk() puts a chunk back in the free list.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    bmem_freechunk    */
/*----------------------*/
void bmem_freechunk(p, q)
    bmem* p;
    void* q;
    {
    if (!p || !q)
        return;
    q = (char*)q - sizeof(char*);
    *(char**)q = p->free;
    p->free = (char*)q;
    } /* End of function bmem_freechunk. */
