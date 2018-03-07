/* src/akmi/rat.h   26 June 1995   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_RAT_H
#define AKMI_RAT_H
/*---------------------------------------------------------------------------
This file defines a class of rational numbers.
Rational numbers are stored in the form i + d/n, where n > 0, and 0 <= d < n,
where d and n are relatively prime.
The integer part is signed 32-bit, but the fractional part is unsigned
16-bit, so that checks can be made on the size of the common denominator
before it is used.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Structs defined in this file:

struct  rat
struct  ratkey
---------------------------------------------------------------------------*/

#include "integer.h"
#include "boolean.h"

/* Structure representing a rational number. */
typedef struct RAT {
    int32 i;            /* Integer part = floor(r). */
    uint16 num, den;    /* Fractional part, always non-negative. */
    } rat;

/* Structure for creating association tables for rats. */
typedef struct RATKEY {
    struct RATKEY* next;
    rat* r;                     /* The rat. */
    int i;                      /* The key. */
    } ratkey;

extern void     rat_ctor();
extern rat*     new_rat();
extern void     rat_delete();
extern int32    rat_floor();
extern uint16   rat_numerator();
extern uint16   rat_denominator();
extern double   rat_double();
extern void     rat_assign_rat();
extern void     rat_assign_short();
extern void     rat_assign_long();
extern void     rat_uminus();
extern int      rat_add_rat();
extern int      rat_mult_rat();
extern long     rat_mult_int();
extern boolean  rat_equals_rat();
extern boolean  rat_greater_rat();
extern double   rat_diff_double();
extern boolean  rat_positive();
extern boolean  rat_equals_long();
extern void     rat_clear();
extern rat*     get_rat();
extern void     rat_fprint();

extern void     rat_array_fprint();

extern void     ratkey_ctor();
extern ratkey*  new_ratkey();
extern void     ratkey_delete();

extern ratkey*  ratkeys_get_rat();

#endif /* AKMI_RAT_H */
