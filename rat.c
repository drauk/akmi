/* src/akmi/rat.c   2009-4-14   Alan Kennington. */
/* $Id$ */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
Functions in this file:

rat_ctor
new_rat
rat_delete
rat_floor
rat_numerator
rat_denominator
rat_double
rat_clear
rat_normal
rat_assign_rat
rat_assign_short
rat_assign_long
rat_uminus
rat_add_rat
rat_mult_rat
rat_mult_int
rat_equals_rat
rat_greater_rat
rat_diff_double
rat_positive
rat_equals_long
get_rat
rat_fprint

rat_array_fprint

ratkey_ctor
new_ratkey
ratkey_delete

ratkeys_get_rat

Note that functions rat_... take rat* as the first parameter.
---------------------------------------------------------------------------*/

#include "rat.h"
#include "bmem.h"
#include "nonstdio.h"
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
/* extern char* malloc(); */

bmem ratmem; /* In C, this has to be initialised by main()! */

/*---------------------------------------------------------------------------
rat_ctor() constructs a rational number.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      rat_ctor        */
/*----------------------*/
void rat_ctor(p)
    rat* p;
    {
    if (!p)
        return;
    p->i = 0;
    p->num = 0;
    p->den = 1;
    } /* End of function rat_ctor. */

/*---------------------------------------------------------------------------
new_rat() returns a new rational number.
---------------------------------------------------------------------------*/
/*----------------------*/
/*        new_rat       */
/*----------------------*/
rat* new_rat()
    {
    rat* p;
/*  p = (rat*)malloc(sizeof(rat)); */
    p = (rat*)bmem_newchunk(&ratmem);
    rat_ctor(p);
    return p;
    } /* End of function new_rat. */

/*---------------------------------------------------------------------------
rat_delete() deletes rational number. There is no extra memory to return.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      rat_delete      */
/*----------------------*/
void rat_delete(p)
    rat* p;
    {
/*  free(p); */
    bmem_freechunk(&ratmem, p);
    } /* End of function rat_delete. */

/*---------------------------------------------------------------------------
rat_floor() returns the floor of a rational number.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      rat_floor       */
/*----------------------*/
int32 rat_floor(p)
    rat* p;
    {
    return p ? p->i : 0;
    } /* End of function rat_floor. */

/*---------------------------------------------------------------------------
rat_numerator() returns the numerator of the fractional part of a
rational number.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     rat_numerator    */
/*----------------------*/
uint16 rat_numerator(p)
    rat* p;
    {
    return p ? p->num : 0;
    } /* End of function rat_numerator. */

/*---------------------------------------------------------------------------
rat_denominator() returns the denominator of the fractional part of a
rational number.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     rat_denominator  */
/*----------------------*/
uint16 rat_denominator(p)
    rat* p;
    {
    return p ? p->den : 1;
    } /* End of function rat_denominator. */

/*---------------------------------------------------------------------------
rat_double() returns the value of the rat, converted to double float.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      rat_double      */
/*----------------------*/
double rat_double(p)
    rat* p;
    {
    double x;

    if (!p)
        return 0;
    x = p->i;
    if (p->den != 0)
        x += p->num/(double)p->den;
    return x;
    } /* End of function rat_double. */

/*---------------------------------------------------------------------------
rat_clear() sets a rational number to 0.
---------------------------------------------------------------------------*/
/*----------------------*/
/*       rat_clear      */
/*----------------------*/
void rat_clear(p)
    rat* p;
    {
    p->i = 0;
    p->num = 0;
    p->den = 1;
    } /* End of function rat_clear. */

/*---------------------------------------------------------------------------
rat_normal() returns a rational number, in normal form, corresponding to
given long integer values for the fields.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      rat_normal      */
/*----------------------*/
static void rat_normal(p, i, n, d)
    rat* p;
    register long i, n, d;
    {
    long t;

    if (!p)
        return;
    if (d == 0) {
        p->i = i;
        p->num = 0;
        p->den = 1;
        return;
        }
    if (d < 0) {
        n = -n;
        d = -d;
        }
    while (d > MAXUINT16) {
        n >>= 1;
        d >>= 1;
        }
    if (n < 0) {
        t = (-n)/d + 1;
        n += t*d;
        i -= t;
        }
    if (n >= d) {
        t = n/d;
        n -= t*d;
        i += t;
        }
    p->i = i;
    p->num = n;
    p->den = d;
    return;
    } /* End of function rat_normal. */

/*---------------------------------------------------------------------------
rat_assign_rat() copies a rational to a rational.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    rat_assign_rat    */
/*----------------------*/
void rat_assign_rat(p, q)
    rat *p, *q;
    {
    if (!p || !q)
        return;
    p->i = q->i;
    p->num = q->num;
    p->den = q->den;
    } /* End of function rat_assign_rat. */

/*---------------------------------------------------------------------------
rat_assign_short() assigns a short integer to a rational.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   rat_assign_short   */
/*----------------------*/
void rat_assign_short(p, i)
    rat* p;
    short i;
    {
    if (!p)
        return;
    p->i = i;
    p->num = 0;
    p->den = 1;
    } /* End of function rat_assign_short. */

/*---------------------------------------------------------------------------
rat_assign_long() assigns a long integer to a rational.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    rat_assign_long   */
/*----------------------*/
void rat_assign_long(p, l)
    rat* p;
    long l;
    {
    if (!p)
        return;
    p->i = l;
    p->num = 0;
    p->den = 1;
    } /* End of function rat_assign_long. */

/*---------------------------------------------------------------------------
rat_uminus() performs a unary minus on a rational.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      rat_uminus      */
/*----------------------*/
void rat_uminus(p)
    rat* p;
    {
    if (!p)
        return;
    p->i = -p->i;
    if (p->num > 0) {
        p->num = p->den - p->num;
        p->i -= 1;
        }
    } /* End of function rat_uminus. */

/*---------------------------------------------------------------------------
rat_add_rat() adds a rational number to a rational number.
The sum is returned in the first argument.
Return value:
0       success
-1      one of the parameters was null
-2      error in a denominator
Note that some sorts of errors are not detected, for the sake of speed.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      rat_add_rat     */
/*----------------------*/
int rat_add_rat(p, q)
    register rat *p, *q;
    {
    register uint16 i1, i2, r;
    register uint32 n, d;
    uint16 t;

    if (!p || !q)
        return -1;

    /* Work out the integer part: */
    p->i += q->i;

    /* Find the lowest common denominator r: */
    i1 = p->den;
    i2 = q->den;
    if (i1 == 0 || i2 == 0)
        return -2;
    if (i1 >= i2) {
        for (;;) {
            if ((i1 %= i2) == 0) {
                r = i2;
                break;
                }
            if ((i2 %= i1) == 0) {
                r = i1;
                break;
                }
            }
        }
    else {
        for (;;) {
            if ((i2 %= i1) == 0) {
                r = i1;
                break;
                }
            if ((i1 %= i2) == 0) {
                r = i2;
                break;
                }
            }
        }

    /* Put   n = p->num * (q->den/r) + q->num * (p->den/r):   */
    t = p->den / r;
    d = q->num * t;
    t = q->den / r;
    n = p->num * t + d;

    /* Put   d = p->den * (q->den/r):   */
    d = p->den;
    d *= t;

    /* Normalise things a bit: */
    while (d > MAXUINT16) {
        n >>= 1;
        d >>= 1;
        }
    while (n >= d) {
        n -= d;
        p->i += 1;
        }
    if (n == 0)
        d = 1;
    else { /* Simplify the fraction: */
        i1 = n;
        i2 = d; /* n and d are known to be non-zero. */
        if (i1 >= i2) {
            for (;;) {
                if ((i1 %= i2) == 0) {
                    r = i2;
                    break;
                    }
                if ((i2 %= i1) == 0) {
                    r = i1;
                    break;
                    }
                }
            }
        else {
            for (;;) {
                if ((i2 %= i1) == 0) {
                    r = i1;
                    break;
                    }
                if ((i1 %= i2) == 0) {
                    r = i2;
                    break;
                    }
                }
            }
        if (r > 1) { /* Case that the fraction can be simplified. */
            n /= r;
            d /= r;
            }
        }
    p->num = n;
    p->den = d;

    return 0;
    } /* End of function rat_add_rat. */

/*---------------------------------------------------------------------------
rat_mult_rat() multiplies a rational number by a rational number.
The product is returned in the first argument.
Return value:
0       success
-1      one of the parameters was null
-2      error in a denominator
Note that some sorts of errors are not detected, for the sake of speed.
Also note that there is not much range compression (factor reduction).
Also, the function could be made more efficient for the redundant cases
---------------------------------------------------------------------------*/
/*----------------------*/
/*     rat_mult_rat     */
/*----------------------*/
int rat_mult_rat(p, q)
    register rat *p, *q;
    {
    register rat* rat1 = new_rat();
    register rat* rat2 = new_rat();

    if (!p || !q)
        return -1;

    if (p->i != 0) {
        if (q->i != 0) {
            rat1->i = p->i * q->i;
            if (q->num != 0) {
                rat_normal(rat2, (long)0, (long)(p->i*q->num), (long)q->den);
                rat_add_rat(rat1, rat2);
                }
            }
        else if (q->num != 0) {
            rat_normal(rat1, (long)0, (long)(p->i*q->num), (long)q->den);
            }
        if (q->i != 0 && p->num != 0) {
            rat_normal(rat2, (long)0, (long)(q->i * p->num), (long)p->den);
            rat_add_rat(rat1, rat2);
            }
        }
    else { /* Case that p->i == 0. */
        if (q->i != 0 && p->num != 0)
            rat_normal(rat1, (long)0, (long)(q->i * p->num), (long)p->den);
        }
    rat_normal(p, (long)0, (long)(p->num*q->num), (long)(p->den*q->den));
    rat_add_rat(p, rat1);
    rat_delete(rat1);
    rat_delete(rat2);

    return 0;
    } /* End of function rat_mult_rat. */

/*---------------------------------------------------------------------------
rat_mult_int() multiplies a rational number by an integer.
The product is returned as the return value.
The first argument is _not_ altered.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     rat_mult_int     */
/*----------------------*/
long rat_mult_int(p, i)
    register rat *p;
    int i;
    {
    if (!p || p->den == 0 || i == 0)
        return 0;

    return (long)p->i * i + ((long)p->num * i)/p->den;
    } /* End of function rat_mult_int. */

/*---------------------------------------------------------------------------
rat_equals_rat() returns true if the first rat is equal to the second rat.
The rats must both be in normal form. I.e. the numerator or each rat must be
less than the denominator.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    rat_equals_rat    */
/*----------------------*/
boolean rat_equals_rat(p, q)
    register rat *p, *q;
    {
    register uint32 l1, l2;

    if (!p && !q)
        return true;
    if (!p || !q)
        return false;
    if (p->i != q->i)
        return false;
    l1 = p->num;
    l1 *= q->den;
    l2 = q->num;
    l2 *= p->den;
    return l1 == l2;
    } /* End of function rat_equals_rat. */

/*---------------------------------------------------------------------------
rat_greater_rat() returns true if the first rat is greater than the
second rat.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    rat_greater_rat   */
/*----------------------*/
boolean rat_greater_rat(p, q)
    register rat *p, *q;
    {
    register uint32 l1, l2;

    if (!p || !q)
        return false;
    if (p->i > q->i)
        return true;
    if (p->i < q->i)
        return false;
    l1 = p->num;
    l1 *= q->den;
    l2 = q->num;
    l2 *= p->den;
    return l1 > l2;
    } /* End of function rat_greater_rat. */

/*---------------------------------------------------------------------------
rat_diff_double() returns the difference of two rats as a double.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    rat_diff_double   */
/*----------------------*/
double rat_diff_double(p, q)
    register rat *p, *q;
    {
    double d;
    register uint32 l1, l2;

    if (!p || !q)
        return 0;

    /* The following code was chosen to maximise speed, without reducing
       correctness. */
    if (p->num == 0) {
        if (q->num == 0)
            return p->i - q->i;
        else {
/*            l1 = q->num;
            d = -l1;
*/
            d = q->num;
            d /= q->den;
            d = -d;
            }
        }
    else if (q->num == 0) {
        l1 = p->num;
        d = l1;
        d /= p->den;
        }
    else {
/*
        l1 = p->num;
        l1 *= q->den;   /* Cross-multiply as uint32, for speed and accuracy. */
/*
        l2 = q->num;
        l2 *= p->den;
        l1 -= l2;
        d = l1;
        l1 = p->den;    /* Multiply denominators as uint32, for sp. and acc. */
/*
        l1 *= q->den;
        d /= l1;
*/
        d = p->num * q->den;
        d -= p->den * q->num;
        d /= p->den * q->den;
        }
    d += p->i;      /* Subtract integer part as double, for correctness. */
    d -= q->i;
    return d;
    } /* End of function rat_diff_double. */

/*---------------------------------------------------------------------------
rat_positive() returns "true" if and only if the rat is positive.
If the rat is a null rat, then "false" is returned.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     rat_positive     */
/*----------------------*/
boolean rat_positive(p)
    rat* p;
    {
    return p && (p->i > 0 || p->num > 0);
    } /* End of function rat_positive. */

/*---------------------------------------------------------------------------
rat_equals_long() returns "true" if and only if the rat is equal to the
given integer. If the rat is a null rat, then "false" is returned.
---------------------------------------------------------------------------*/
/*----------------------*/
/*    rat_equals_long   */
/*----------------------*/
boolean rat_equals_long(p, i)
    rat* p;
    long i;
    {
    return p && p->num == 0 && p->i == i;
    } /* End of function rat_equals_long. */

/*---------------------------------------------------------------------------
get_rat() reads a rational number from a file. If the file is stdin, then
the characters '/', '-' and '+' may not appear at the beginning of a line.
A pointer to a new rat is always returned. The default value is 1, if there
is nothing recognisable at the current position in the stream.
---------------------------------------------------------------------------*/
/*----------------------*/
/*        get_rat       */
/*----------------------*/
rat* get_rat(f)
    register FILE* f;
    {
    register rat* p = new_rat();
    long l1 = 0, l2 = 0, l3 = 0;
    register int c;

    if (!f)
        return p;
    if ((c = inch(f)) == EOF)
        return p;
    if (c == '/') {                 /* Format "/ l1". */
        fscanf(f, "%ld", &l1);
        rat_normal(p, (long)0, (long)1, l1);
        return p;
        }
    ungetc((char)c, f);
                                    /* Format "" */
    if (!isascii(c) || !(isdigit(c) || c == '-' || c == '+')) {
        p->i = 1;
        return p;
        }

    fscanf(f, "%ld", &l1);
    if ((c = inch(f)) == EOF) {     /* Format "l1". */
        rat_assign_long(p, l1);
        return p;
        }
    if (c == '/') {                 /* Format "l1 / l2". */
        fscanf(f, "%ld", &l2);
        rat_normal(p, (long)0, l1, l2);
        return p;
        }
    else if (c == '+') {
        fscanf(f, "%ld", &l2);
        if ((c = inch(f)) == EOF) { /* Format "l1 + l2". */
            rat_assign_long(p, l1 + l2);
            return p;
            }
        if (c == '/') {             /* Format "l1 + l2 / l3". */
            fscanf(f, "%ld", &l3);
            rat_normal(p, l1, l2, l3);
            return p;
            }
        else {                      /* Format "l1 + l2". */
            ungetc((char)c, f);
            rat_assign_long(p, l1 + l2);
            return p;
            }
        }
    else if (c == '-') {
        fscanf(f, "%ld", &l2);
        if ((c = inch(f)) == EOF) { /* Format "l1 - l2". */
            rat_assign_long(p, l1 - l2);
            return p;
            }
        if (c == '/') {             /* Format "l1 - l2 / l3". */
            fscanf(f, "%ld", &l3);
            rat_normal(p, l1, -l2, l3);
            return p;
            }
        else {                      /* Format "l1 - l2". */
            ungetc((char)c, f);
            rat_assign_long(p, l1 - l2);
            return p;
            }
        }
    else {                          /* Format "l1". */
        ungetc((char)c, f);
        rat_assign_long(p, l1);
        return p;
        }
    } /* End of function get_rat. */

/*---------------------------------------------------------------------------
rat_fprint() prints a rational number to a file.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      rat_fprint      */
/*----------------------*/
void rat_fprint(p, f)
    rat* p;
    FILE* f;
    {
    if (!f || !p)
        return;
    if (p->num == 0) {          /* r = i + 0/d. */
        fprintf(f, "%ld", p->i);
        return;
        }
    if (p->i == 0) {            /* r = 0 + n/d. */
        fprintf(f, "%ld/%ld", (uint32)p->num, (uint32)p->den);
        return;
        }
    /* r = i + n/d: */
    fprintf(f, "%ld + %ld/%ld", p->i, (uint32)p->num, (uint32)p->den);
    } /* End of function rat_fprint. */

/*---------------------------------------------------------------------------
rat_array_fprint() is an ad hoc routine to print an array of rats.
---------------------------------------------------------------------------*/
/*----------------------*/
/*   rat_array_fprint   */
/*----------------------*/
void rat_array_fprint(p, n, f)
    rat* p;
    int n;
    FILE* f;
    {
    int i;

    fprintf(f, "The array of rats is:\nrat\tvalue\n");
    for (i = 0; i < n; ++i) {
        fprintf(f, "%d\t", i);
        rat_fprint(p++, f);
        fprintf(f, "\n");
        }
    } /* End of function rat_array_fprint. */

/*---------------------------------------------------------------------------
ratkey_ctor() constructs a rat-key.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      ratkey_ctor     */
/*----------------------*/
void ratkey_ctor(p)
    ratkey* p;
    {
    if (!p)
        return;
    p->next = 0;
    p->r = 0;
    p->i = 0;
    } /* End of function ratkey_ctor. */

/*---------------------------------------------------------------------------
new_ratkey() returns a new ratkey structure.
---------------------------------------------------------------------------*/
/*----------------------*/
/*      new_ratkey      */
/*----------------------*/
ratkey* new_ratkey()
    {
    ratkey* p;
    p = (ratkey*)malloc(sizeof(ratkey));
    ratkey_ctor(p);
    return p;
    } /* End of function new_ratkey. */

/*---------------------------------------------------------------------------
ratkey_delete() deletes a ratkey structure.
---------------------------------------------------------------------------*/
/*----------------------*/
/*     ratkey_delete    */
/*----------------------*/
void ratkey_delete(p)
    ratkey* p;
    {
    if (!p)
        return;
    if (p->r)
        rat_delete(p->r);
    free(p);
    } /* End of function ratkey_delete. */

/*------------------------------------------------------------------------------
ratkeys_get_rat() gets a ratkey with a given rat value. The ratkeys list is not
associated with a free list. If the given value is not in the list, then a new
ratkey is allocated, and it is given the index value corresponding to its order
of allocation, not according to its position in the list.
------------------------------------------------------------------------------*/
/*----------------------*/
/*    ratkeys_get_rat   */
/*----------------------*/
ratkey* ratkeys_get_rat(ratkeys, value)
    ratkey** ratkeys;
    rat* value;
    {
    int idx = 0;
    ratkey* px;

    if (!ratkeys || !value)
        return 0;
    for (px = *ratkeys, idx = 0; px; px = px->next, ++idx)
        if (rat_equals_rat(px->r, value))
            break;
    if (!px) {
        px = new_ratkey();
        px->r = new_rat();
        rat_assign_rat(px->r, value);
        px->i = idx;
        px->next = *ratkeys;
        *ratkeys = px;
        }
    return px;
    } /* End of function ratkeys_get_rat. */
