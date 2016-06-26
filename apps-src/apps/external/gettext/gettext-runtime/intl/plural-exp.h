/* Expression parsing and evaluation for plural form selection.
   Copyright (C) 2000-2003, 2005 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@cygnus.com>, 2000.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.  */

#ifndef _PLURAL_EXP_H
#define _PLURAL_EXP_H

#ifndef internal_function
# define internal_function
#endif

#ifndef attribute_hidden
# define attribute_hidden
#endif


/* This is the representation of the expressions to determine the
   plural form.  */
struct expression
{
  int nargs;			/* Number of arguments.  */
  enum operator
  {
    /* Without arguments:  */
    var,			/* The variable "n".  */
    num,			/* Decimal number.  */
    /* Unary operators:  */
    lnot,			/* Logical NOT.  */
    /* Binary operators:  */
    mult,			/* Multiplication.  */
    divide,			/* Division.  */
    module,			/* Modulo operation.  */
    plus,			/* Addition.  */
    minus,			/* Subtraction.  */
    less_than,			/* Comparison.  */
    greater_than,		/* Comparison.  */
    less_or_equal,		/* Comparison.  */
    greater_or_equal,		/* Comparison.  */
    equal,			/* Comparison for equality.  */
    not_equal,			/* Comparison for inequality.  */
    land,			/* Logical AND.  */
    lor,			/* Logical OR.  */
    /* Ternary operators:  */
    qmop			/* Question mark operator.  */
  } operation;
  union
  {
    unsigned long int num;	/* Number value for `num'.  */
    struct expression *args[3];	/* Up to three arguments.  */
  } val;
};

/* This is the data structure to pass information to the parser and get
   the result in a thread-safe way.  */
struct parse_args
{
  const char *cp;
  struct expression *res;
};


/* Names for the libintl functions are a problem.  This source code is used
   1. in the GNU C Library library,
   2. in the GNU libintl library,
   3. in the GNU gettext tools.
   The function names in each situation must be different, to allow for
   binary incompatible changes in 'struct expression'.  Furthermore,
   1. in the GNU C Library library, the names have a __ prefix,
   2.+3. in the GNU libintl library and in the GNU gettext tools, the names
         must follow ANSI C and not start with __.
   So we have to distinguish the three cases.  */
# define FREE_EXPRESSION libintl_gettext_free_exp
# define PLURAL_PARSE libintl_gettextparse
# define GERMANIC_PLURAL libintl_gettext_germanic_plural
# define EXTRACT_PLURAL_EXPRESSION libintl_gettext_extract_plural

extern void FREE_EXPRESSION (struct expression *exp)
     internal_function;
extern int PLURAL_PARSE (void *arg);
extern struct expression GERMANIC_PLURAL attribute_hidden;
extern void EXTRACT_PLURAL_EXPRESSION (const char *nullentry,
				       struct expression **pluralp,
				       unsigned long int *npluralsp)
     internal_function;

#endif /* _PLURAL_EXP_H */
