/* src/akmi/integer.h   24 June 1995   Alan U. Kennington. */
/*----------------------------------------------------------------------------
Copyright (C) 1999, Alan Kennington.
You may distribute this software under the terms of Alan Kennington's
modified Artistic Licence, as specified in the accompanying LICENCE file.
----------------------------------------------------------------------------*/
#ifndef AKMI_INTEGER_H
#define AKMI_INTEGER_H
/*---------------------------------------------------------------------------
This file defines various types of integers.
On the 68000, a "char" is signed, as far as I know, and causes sign-extension
when assigned to a larger integer. (?)
---------------------------------------------------------------------------*/

typedef char            int8;
typedef short           int16;
typedef long            int32;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;
#define MAXINT8     127
#define MAXINT16    32767
#define MAXINT32    2147483647
#define MAXUINT8    255
#define MAXUINT16   65535
#define MAXUINT32   4294967295

#endif /* AKMI_INTEGER_H */
