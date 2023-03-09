/*
 * ckconfig.c
 *
 * Copyright (C) 1991-1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 */

/*
 * This program is intended to help you determine how to configure the JPEG
 * software for installation on a particular system.  The idea is to try to
 * compile and execute this program.  If your compiler fails to compile the
 * program, make changes as indicated in the comments below.  Once you can
 * compile the program, run it, and it will produce a "jconfig.h" file for
 * your system.
 *
 * As a general rule, each time you try to compile this program,
 * pay attention only to the *first* error message you get from the compiler.
 * Many C compilers will issue lots of spurious error messages once they
 * have gotten confused.  Go to the line indicated in the first error message,
 * and read the comments preceding that line to see what to change.
 *
 * Almost all of the edits you may need to make to this program consist of
 * changing a line that reads "#define SOME_SYMBOL" to "#undef SOME_SYMBOL",
 * or vice versa.  This is called defining or undefining that symbol.
 */

#define _CRT_SECURE_NO_WARNINGS

/* First we must see if your system has the include files we need.
 * We start out with the assumption that your system has all the ANSI-standard
 * include files.  If you get any error trying to include one of these files,
 * undefine the corresponding HAVE_xxx symbol.
 */

#define HAVE_STDDEF_H		/* replace 'define' by 'undef' if error here */
#ifdef HAVE_STDDEF_H		/* next line will be skipped if you undef... */
#include <stddef.h>
#endif

#define HAVE_STDLIB_H		/* same thing for stdlib.h */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <stdio.h>		/* If you ain't got this, you ain't got C. */

/* We have to see if your string functions are defined by
 * strings.h (old BSD convention) or string.h (everybody else).
 * We try the non-BSD convention first; define NEED_BSD_STRINGS
 * if the compiler says it can't find string.h.
 */

#undef NEED_BSD_STRINGS

#ifdef NEED_BSD_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

/* On some systems (especially older Unix machines), type size_t is
 * defined only in the include file <sys/types.h>.  If you get a failure
 * on the size_t test below, try defining NEED_SYS_TYPES_H.
 */

#undef NEED_SYS_TYPES_H		/* start by assuming we don't need it */
#ifdef NEED_SYS_TYPES_H
#include <sys/types.h>
#endif


/* Usually type size_t is defined in one of the include files we've included
 * above.  If not, you'll get an error on the "typedef size_t my_size_t;" line.
 * In that case, first try defining NEED_SYS_TYPES_H just above.
 * If that doesn't work, you'll have to search through your system library
 * to figure out which include file defines "size_t".  Look for a line that
 * says "typedef something-or-other size_t;".  Then, change the line below
 * that says "#include <someincludefile.h>" to instead include the file
 * you found size_t in, and define NEED_SPECIAL_INCLUDE.  If you can't find
 * type size_t anywhere, try replacing "#include <someincludefile.h>" with
 * "typedef unsigned int size_t;".
 */

#undef NEED_SPECIAL_INCLUDE	/* assume we DON'T need it, for starters */

#ifdef NEED_SPECIAL_INCLUDE
#include <someincludefile.h>
#endif

typedef size_t my_size_t;	/* The payoff: do we have size_t now? */


/* The next question is whether your compiler supports ANSI-style function
 * prototypes.  You need to know this in order to choose between using
 * makefile.ansi and using makefile.unix.
 * The #define line below is set to assume you have ANSI function prototypes.
 * If you get an error in this group of lines, undefine HAVE_PROTOTYPES.
 */

#define HAVE_PROTOTYPES

#ifdef HAVE_PROTOTYPES
int testfunction (int arg1, int * arg2); /* check prototypes */

struct methods_struct {		/* check method-pointer declarations */
  int (*error_exit) (char *msgtext);
  int (*trace_message) (char *msgtext);
  int (*another_method) (void);
};

int testfunction (int arg1, int * arg2) /* check definitions */
{
  return arg2[arg1];
}

int test2function (void)	/* check void arg list */
{
  return 0;
}
#endif


/* Now we want to find out if your compiler knows what "unsigned char" means.
 * If you get an error on the "unsigned char un_char;" line,
 * then undefine HAVE_UNSIGNED_CHAR.
 */

#define HAVE_UNSIGNED_CHAR

#ifdef HAVE_UNSIGNED_CHAR
unsigned char un_char;
#endif


/* Now we want to find out if your compiler knows what "unsigned short" means.
 * If you get an error on the "unsigned short un_short;" line,
 * then undefine HAVE_UNSIGNED_SHORT.
 */

#define HAVE_UNSIGNED_SHORT

#ifdef HAVE_UNSIGNED_SHORT
unsigned short un_short;
#endif


/* Now we want to find out if your compiler understands type "void".
 * If you get an error anywhere in here, undefine HAVE_VOID.
 */

#define HAVE_VOID

#ifdef HAVE_VOID
/* Caution: a C++ compiler will insist on complete prototypes */
typedef void * void_ptr;	/* check void * */
#ifdef HAVE_PROTOTYPES		/* check ptr to function returning void */
typedef void (*void_func) (int a, int b);
#else
typedef void (*void_func) ();
#endif

#ifdef HAVE_PROTOTYPES		/* check void function result */
void test3function (void_ptr arg1, void_func arg2)
#else
void test3function (arg1, arg2)
     void_ptr arg1;
     void_func arg2;
#endif
{
  char * locptr = (char *) arg1; /* check casting to and from void * */
  arg1 = (void *) locptr;
  (*arg2) (1, 2);		/* check call of fcn returning void */
}
#endif


/* Now we want to find out if your compiler knows what "const" means.
 * If you get an error here, undefine HAVE_CONST.
 */

#define HAVE_CONST

#ifdef HAVE_CONST
static const int carray[3] = {1, 2, 3};

#ifdef HAVE_PROTOTYPES
int test4function (const int arg1)
#else
int test4function (arg1)
     const int arg1;
#endif
{
  return carray[arg1];
}
#endif


/* If you get an error or warning about this structure definition,
 * define INCOMPLETE_TYPES_BROKEN.
 */

#undef INCOMPLETE_TYPES_BROKEN

#ifndef INCOMPLETE_TYPES_BROKEN
typedef struct undefined_structure * undef_struct_ptr;
#endif


/* If you get an error about duplicate names,
 * define NEED_SHORT_EXTERNAL_NAMES.
 */

#undef NEED_SHORT_EXTERNAL_NAMES

#ifndef NEED_SHORT_EXTERNAL_NAMES

int possibly_duplicate_function ()
{
  return 0;
}

int possibly_dupli_function ()
{
  return 1;
}

#endif



/************************************************************************
 *  OK, that's it.  You should not have to change anything beyond this
 *  point in order to compile and execute this program.  (You might get
 *  some warnings, but you can ignore them.)
 *  When you run the program, it will make a couple more tests that it
 *  can do automatically, and then it will create jconfig.h and print out
 *  any additional suggestions it has.
 ************************************************************************
 */


#ifdef HAVE_PROTOTYPES
int is_char_signed (int arg)
#else
int is_char_signed (arg)
     int arg;
#endif
{
  if (arg == 189) {		/* expected result for unsigned char */
    return 0;			/* type char is unsigned */
  }
  else if (arg != -67) {	/* expected result for signed char */
    printf("Hmm, it seems 'char' is not eight bits wide on your machine.\n");
    printf("I fear the JPEG software will not work at all.\n\n");
  }
  return 1;			/* assume char is signed otherwise */
}


#ifdef HAVE_PROTOTYPES
int is_shifting_signed (long arg)
#else
int is_shifting_signed (arg)
     long arg;
#endif
/* See whether right-shift on a long is signed or not. */
{
  long res = arg >> 4;

  if (res == -0x7F7E80CL) {	/* expected result for signed shift */
    return 1;			/* right shift is signed */
  }
  /* see if unsigned-shift hack will fix it. */
  /* we can't just test exact value since it depends on width of long... */
  res |= (~0L) << (32-4);
  if (res == -0x7F7E80CL) {	/* expected result now? */
    return 0;			/* right shift is unsigned */
  }
  printf("Right shift isn't acting as I expect it to.\n");
  printf("I fear the JPEG software will not work at all.\n\n");
  return 0;			/* try it with unsigned anyway */
}