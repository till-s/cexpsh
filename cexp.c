
/*  A Bison parser, made from cexp.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	NUMBER	257
#define	IDENT	258
#define	VAR	259
#define	FUNC	260
#define	CHAR_VAR	261
#define	SHORT_VAR	262
#define	LABEL	263
#define	CHAR_CAST	264
#define	SHORT_CAST	265
#define	LONG_CAST	266
#define	NONE	267
#define	EQ	268
#define	NE	269
#define	LE	270
#define	GE	271
#define	SHFT	272
#define	NEG	273
#define	CAST	274
#define	ADDR	275
#define	DEREF	276
#define	CALL	277

#line 1 "cexp.y"

#include <stdio.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#define _INSIDE_CEXP_Y
#include "cexp.h"
#undef  _INSIDE_CEXP_Y
#define BOOLTRUE	1
#define YYPARSE_PARAM		parm
#define YYLEX_PARAM		parm
#define YYERROR_VERBOSE

#line 16 "cexp.y"
typedef union {
	unsigned long	num;	/* a number */
	unsigned char	*caddr;	/* an byte address */
	unsigned short	*saddr;	/* an short address */
	unsigned long	*laddr;	/* an long address */
	unsigned char	*id;	/* an undefined identifier */
	CexpFuncPtr		func;	/* function  pointer */
	CexpSym			sym;	/* a symbol table entry */
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		136
#define	YYFLAG		-32768
#define	YYNTBASE	41

#define YYTRANSLATE(x) ((unsigned)(x) <= 277 ? yytranslate[x] : 54)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    37,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    30,     2,     2,     2,    29,    17,     2,    38,
    39,    27,    26,    40,    25,     2,    28,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    20,
    14,    21,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    16,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    15,     2,    31,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    18,    19,    22,
    23,    24,    32,    33,    34,    35,    36
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     7,     9,    11,    13,    15,    17,    19,
    21,    23,    27,    31,    35,    39,    43,    47,    49,    54,
    59,    63,    67,    71,    75,    79,    82,    85,    89,    91,
    93,    97,   102,   109,   118,   129,   142,   157,   174,   193,
   214,   237,   240,   244,   249,   254,   259,   264,   268,   270,
   272,   274,   276,   281,   284,   286,   291,   294,   296,   301,
   304,   307,   313,   316,   322,   325,   331
};

static const short yyrhs[] = {    42,
     0,    37,     0,    43,    37,     0,     3,     0,     9,     0,
    51,     0,    52,     0,    53,     0,    48,     0,    49,     0,
    50,     0,    48,    14,    43,     0,    49,    14,    43,     0,
    50,    14,    43,     0,    43,    15,    43,     0,    43,    16,
    43,     0,    43,    17,    43,     0,    46,     0,    43,    20,
    20,    43,     0,    43,    21,    21,    43,     0,    43,    26,
    43,     0,    43,    25,    43,     0,    43,    27,    43,     0,
    43,    28,    43,     0,    43,    29,    43,     0,    25,    43,
     0,    31,    43,     0,    38,    43,    39,     0,    45,     0,
     6,     0,    44,    38,    39,     0,    44,    38,    43,    39,
     0,    44,    38,    43,    40,    43,    39,     0,    44,    38,
    43,    40,    43,    40,    43,    39,     0,    44,    38,    43,
    40,    43,    40,    43,    40,    43,    39,     0,    44,    38,
    43,    40,    43,    40,    43,    40,    43,    40,    43,    39,
     0,    44,    38,    43,    40,    43,    40,    43,    40,    43,
    40,    43,    40,    43,    39,     0,    44,    38,    43,    40,
    43,    40,    43,    40,    43,    40,    43,    40,    43,    40,
    43,    39,     0,    44,    38,    43,    40,    43,    40,    43,
    40,    43,    40,    43,    40,    43,    40,    43,    40,    43,
    39,     0,    44,    38,    43,    40,    43,    40,    43,    40,
    43,    40,    43,    40,    43,    40,    43,    40,    43,    40,
    43,    39,     0,    44,    38,    43,    40,    43,    40,    43,
    40,    43,    40,    43,    40,    43,    40,    43,    40,    43,
    40,    43,    40,    43,    39,     0,    30,    43,     0,    43,
    20,    43,     0,    43,    20,    14,    43,     0,    43,    14,
    14,    43,     0,    43,    30,    14,    43,     0,    43,    21,
    14,    43,     0,    43,    21,    43,     0,     7,     0,     8,
     0,     5,     0,     7,     0,    38,    10,    39,    47,     0,
    27,    51,     0,     8,     0,    38,    11,    39,    47,     0,
    27,    52,     0,     5,     0,    38,    12,    39,    47,     0,
    27,    53,     0,    17,     7,     0,    38,    10,    27,    39,
    43,     0,    17,     8,     0,    38,    11,    27,    39,    43,
     0,    17,     5,     0,    38,    12,    27,    39,    43,     0,
     6,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    60,    62,    63,    66,    67,    68,    69,    70,    71,    72,
    73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
    83,    84,    85,    86,    87,    88,    89,    90,    91,    94,
    97,    99,   101,   103,   105,   107,   109,   111,   113,   115,
   117,   121,   122,   123,   124,   125,   126,   127,   130,   131,
   132,   136,   137,   138,   140,   141,   142,   144,   145,   146,
   149,   150,   153,   154,   157,   158,   159
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","NUMBER",
"IDENT","VAR","FUNC","CHAR_VAR","SHORT_VAR","LABEL","CHAR_CAST","SHORT_CAST",
"LONG_CAST","NONE","'='","'|'","'^'","'&'","EQ","NE","'<'","'>'","LE","GE","SHFT",
"'-'","'+'","'*'","'/'","'%'","'!'","'~'","NEG","CAST","ADDR","DEREF","CALL",
"'\\n'","'('","')'","','","input","line","exp","funcp","call","bool","var","clval",
"slval","llval","cptr","sptr","lptr", NULL
};
#endif

static const short yyr1[] = {     0,
    41,    42,    42,    43,    43,    43,    43,    43,    43,    43,
    43,    43,    43,    43,    43,    43,    43,    43,    43,    43,
    43,    43,    43,    43,    43,    43,    43,    43,    43,    44,
    45,    45,    45,    45,    45,    45,    45,    45,    45,    45,
    45,    46,    46,    46,    46,    46,    46,    46,    47,    47,
    47,    48,    48,    48,    49,    49,    49,    50,    50,    50,
    51,    51,    52,    52,    53,    53,    53
};

static const short yyr2[] = {     0,
     1,     1,     2,     1,     1,     1,     1,     1,     1,     1,
     1,     3,     3,     3,     3,     3,     3,     1,     4,     4,
     3,     3,     3,     3,     3,     2,     2,     3,     1,     1,
     3,     4,     6,     8,    10,    12,    14,    16,    18,    20,
    22,     2,     3,     4,     4,     4,     4,     3,     1,     1,
     1,     1,     4,     2,     1,     4,     2,     1,     4,     2,
     2,     5,     2,     5,     2,     5,     1
};

static const short yydefact[] = {     0,
     4,    58,    67,    52,    55,     5,     0,     0,     0,     0,
     0,     2,     0,     1,     0,     0,    29,    18,     9,    10,
    11,     6,     7,     8,    65,    61,    63,    26,    67,     0,
    54,    57,    60,    42,    27,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     3,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    28,     0,    15,    16,    17,
     0,     0,    43,     0,     0,    48,    22,    21,    23,    24,
    25,     0,    31,     0,    12,    13,    14,     0,    51,    49,
    50,    53,     0,    56,     0,    59,    45,    44,    19,    47,
    20,    46,    32,     0,    62,    64,    66,     0,    33,     0,
     0,    34,     0,     0,    35,     0,     0,    36,     0,     0,
    37,     0,     0,    38,     0,     0,    39,     0,     0,    40,
     0,     0,    41,     0,     0,     0
};

static const short yydefgoto[] = {   134,
    14,    15,    16,    17,    18,    92,    19,    20,    21,    22,
    23,    24
};

static const short yypact[] = {   205,
-32768,-32768,   -37,-32768,-32768,-32768,    53,   220,     5,   220,
   220,-32768,   156,-32768,   426,   -32,-32768,-32768,    -7,    -4,
     3,-32768,-32768,-32768,-32768,-32768,-32768,    14,-32768,    87,
-32768,-32768,-32768,    14,    14,   -23,   -19,   -18,   392,    35,
   220,   220,   220,    48,   171,   220,   220,   220,   220,   220,
    56,-32768,   119,   220,   220,   220,    49,    50,    54,    43,
    64,    55,    64,    61,    64,-32768,   220,   113,   440,    63,
   220,   220,   126,   220,   220,   126,     2,     2,    14,    14,
    14,   220,-32768,   239,    -2,    -2,    -2,   220,-32768,-32768,
-32768,-32768,   220,-32768,   220,-32768,    63,   126,   126,   126,
   126,    63,-32768,   220,    14,    14,    14,   256,-32768,   220,
   273,-32768,   220,   290,-32768,   220,   307,-32768,   220,   324,
-32768,   220,   341,-32768,   220,   358,-32768,   220,   375,-32768,
   220,   409,-32768,    95,   101,-32768
};

static const short yypgoto[] = {-32768,
-32768,    -8,-32768,-32768,-32768,   -13,-32768,-32768,-32768,    94,
    97,    98
};


#define	YYLAST		470


static const short yytable[] = {    28,
   -30,    34,    35,    60,    39,    53,    54,    62,    64,    55,
    29,    40,    41,    42,    43,    61,    56,    44,    45,    63,
    65,     7,    46,    47,    48,    49,    50,    51,    48,    49,
    50,    51,    68,    69,    70,    73,    76,    77,    78,    79,
    80,    81,    30,    51,    84,    85,    86,    87,    67,    94,
     1,    96,     2,     3,     4,     5,     6,    25,    97,    26,
    27,    71,    98,    99,     7,   100,   101,    72,    89,    82,
    90,    91,     8,   102,     9,    60,    62,    10,    11,   105,
    64,    88,    44,    45,   106,    13,   107,    46,    47,    48,
    49,    50,    51,    93,   135,   108,    57,    58,    59,    95,
   136,   111,    31,     0,   114,    32,    33,   117,     0,     0,
   120,     0,     0,   123,     0,     0,   126,     0,     0,   129,
     0,     1,   132,     2,     3,     4,     5,     6,    42,    43,
     0,     0,    44,    45,     0,     7,     0,    46,    47,    48,
    49,    50,    51,     8,     0,     9,     0,     0,    10,    11,
    46,    47,    48,    49,    50,    51,    13,    83,     1,     0,
     2,     3,     4,     5,     6,    36,    37,    38,     0,     0,
     0,     0,     7,     1,     0,     2,     3,     4,     5,     6,
     8,     0,     9,     0,    74,    10,    11,     7,     0,     0,
     0,    75,     0,    13,     0,     8,     0,     9,     0,     0,
    10,    11,     0,     0,     0,     0,     0,     1,    13,     2,
     3,     4,     5,     6,     0,     0,     0,     0,     0,     0,
     0,     7,     1,     0,     2,     3,     4,     5,     6,     8,
     0,     9,     0,     0,    10,    11,     7,     0,     0,     0,
     0,    12,    13,     0,     8,     0,     9,     0,     0,    10,
    11,     0,    40,    41,    42,    43,     0,    13,    44,    45,
     0,     0,     0,    46,    47,    48,    49,    50,    51,    40,
    41,    42,    43,     0,     0,    44,    45,   103,   104,     0,
    46,    47,    48,    49,    50,    51,    40,    41,    42,    43,
     0,     0,    44,    45,   109,   110,     0,    46,    47,    48,
    49,    50,    51,    40,    41,    42,    43,     0,     0,    44,
    45,   112,   113,     0,    46,    47,    48,    49,    50,    51,
    40,    41,    42,    43,     0,     0,    44,    45,   115,   116,
     0,    46,    47,    48,    49,    50,    51,    40,    41,    42,
    43,     0,     0,    44,    45,   118,   119,     0,    46,    47,
    48,    49,    50,    51,    40,    41,    42,    43,     0,     0,
    44,    45,   121,   122,     0,    46,    47,    48,    49,    50,
    51,    40,    41,    42,    43,     0,     0,    44,    45,   124,
   125,     0,    46,    47,    48,    49,    50,    51,    40,    41,
    42,    43,     0,     0,    44,    45,   127,   128,     0,    46,
    47,    48,    49,    50,    51,    40,    41,    42,    43,     0,
     0,    44,    45,   130,   131,     0,    46,    47,    48,    49,
    50,    51,    40,    41,    42,    43,     0,     0,    44,    45,
    66,     0,     0,    46,    47,    48,    49,    50,    51,    40,
    41,    42,    43,     0,     0,    44,    45,   133,     0,     0,
    46,    47,    48,    49,    50,    51,    43,     0,     0,    44,
    45,     0,    52,     0,    46,    47,    48,    49,    50,    51
};

static const short yycheck[] = {     8,
    38,    10,    11,    27,    13,    38,    14,    27,    27,    14,
     6,    14,    15,    16,    17,    39,    14,    20,    21,    39,
    39,    17,    25,    26,    27,    28,    29,    30,    27,    28,
    29,    30,    41,    42,    43,    44,    45,    46,    47,    48,
    49,    50,    38,    30,    53,    54,    55,    56,    14,    63,
     3,    65,     5,     6,     7,     8,     9,     5,    67,     7,
     8,    14,    71,    72,    17,    74,    75,    20,     5,    14,
     7,     8,    25,    82,    27,    27,    27,    30,    31,    88,
    27,    39,    20,    21,    93,    38,    95,    25,    26,    27,
    28,    29,    30,    39,     0,   104,    10,    11,    12,    39,
     0,   110,     9,    -1,   113,     9,     9,   116,    -1,    -1,
   119,    -1,    -1,   122,    -1,    -1,   125,    -1,    -1,   128,
    -1,     3,   131,     5,     6,     7,     8,     9,    16,    17,
    -1,    -1,    20,    21,    -1,    17,    -1,    25,    26,    27,
    28,    29,    30,    25,    -1,    27,    -1,    -1,    30,    31,
    25,    26,    27,    28,    29,    30,    38,    39,     3,    -1,
     5,     6,     7,     8,     9,    10,    11,    12,    -1,    -1,
    -1,    -1,    17,     3,    -1,     5,     6,     7,     8,     9,
    25,    -1,    27,    -1,    14,    30,    31,    17,    -1,    -1,
    -1,    21,    -1,    38,    -1,    25,    -1,    27,    -1,    -1,
    30,    31,    -1,    -1,    -1,    -1,    -1,     3,    38,     5,
     6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    17,     3,    -1,     5,     6,     7,     8,     9,    25,
    -1,    27,    -1,    -1,    30,    31,    17,    -1,    -1,    -1,
    -1,    37,    38,    -1,    25,    -1,    27,    -1,    -1,    30,
    31,    -1,    14,    15,    16,    17,    -1,    38,    20,    21,
    -1,    -1,    -1,    25,    26,    27,    28,    29,    30,    14,
    15,    16,    17,    -1,    -1,    20,    21,    39,    40,    -1,
    25,    26,    27,    28,    29,    30,    14,    15,    16,    17,
    -1,    -1,    20,    21,    39,    40,    -1,    25,    26,    27,
    28,    29,    30,    14,    15,    16,    17,    -1,    -1,    20,
    21,    39,    40,    -1,    25,    26,    27,    28,    29,    30,
    14,    15,    16,    17,    -1,    -1,    20,    21,    39,    40,
    -1,    25,    26,    27,    28,    29,    30,    14,    15,    16,
    17,    -1,    -1,    20,    21,    39,    40,    -1,    25,    26,
    27,    28,    29,    30,    14,    15,    16,    17,    -1,    -1,
    20,    21,    39,    40,    -1,    25,    26,    27,    28,    29,
    30,    14,    15,    16,    17,    -1,    -1,    20,    21,    39,
    40,    -1,    25,    26,    27,    28,    29,    30,    14,    15,
    16,    17,    -1,    -1,    20,    21,    39,    40,    -1,    25,
    26,    27,    28,    29,    30,    14,    15,    16,    17,    -1,
    -1,    20,    21,    39,    40,    -1,    25,    26,    27,    28,
    29,    30,    14,    15,    16,    17,    -1,    -1,    20,    21,
    39,    -1,    -1,    25,    26,    27,    28,    29,    30,    14,
    15,    16,    17,    -1,    -1,    20,    21,    39,    -1,    -1,
    25,    26,    27,    28,    29,    30,    17,    -1,    -1,    20,
    21,    -1,    37,    -1,    25,    26,    27,    28,    29,    30
};
#define YYPURE 1

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 60 "cexp.y"
{ YYACCEPT; ;
    break;}
case 3:
#line 63 "cexp.y"
{ printf("0x%08x (%i)\n",yyvsp[-1].num,yyvsp[-1].num); ;
    break;}
case 4:
#line 66 "cexp.y"
{ yyval.num=yyvsp[0].num; ;
    break;}
case 5:
#line 67 "cexp.y"
{ yyval.num=(unsigned long)yyvsp[0].sym->val.addr; ;
    break;}
case 6:
#line 68 "cexp.y"
{ yyval.num=(unsigned long)yyvsp[0].caddr; ;
    break;}
case 7:
#line 69 "cexp.y"
{ yyval.num=(unsigned long)yyvsp[0].saddr; ;
    break;}
case 8:
#line 70 "cexp.y"
{ yyval.num=(unsigned long)yyvsp[0].laddr; ;
    break;}
case 9:
#line 71 "cexp.y"
{ yyval.num=*yyvsp[0].caddr; ;
    break;}
case 10:
#line 72 "cexp.y"
{ yyval.num=*yyvsp[0].saddr; ;
    break;}
case 11:
#line 73 "cexp.y"
{ yyval.num=*yyvsp[0].laddr; ;
    break;}
case 12:
#line 74 "cexp.y"
{ yyval.num=yyvsp[0].num; *yyvsp[-2].caddr=(unsigned char)yyvsp[0].num; ;
    break;}
case 13:
#line 75 "cexp.y"
{ yyval.num=yyvsp[0].num; *yyvsp[-2].saddr=(unsigned short)yyvsp[0].num; ;
    break;}
case 14:
#line 76 "cexp.y"
{ yyval.num=yyvsp[0].num; *yyvsp[-2].laddr=(unsigned long)yyvsp[0].num; ;
    break;}
case 15:
#line 77 "cexp.y"
{ yyval.num=yyvsp[-2].num|yyvsp[0].num; ;
    break;}
case 16:
#line 78 "cexp.y"
{ yyval.num=yyvsp[-2].num^yyvsp[0].num; ;
    break;}
case 17:
#line 79 "cexp.y"
{ yyval.num=yyvsp[-2].num&yyvsp[0].num; ;
    break;}
case 18:
#line 80 "cexp.y"
{ yyval.num=yyvsp[0].num; ;
    break;}
case 19:
#line 81 "cexp.y"
{ yyval.num=(yyvsp[-3].num<<yyvsp[0].num); ;
    break;}
case 20:
#line 82 "cexp.y"
{ yyval.num=(yyvsp[-3].num>>yyvsp[0].num); ;
    break;}
case 21:
#line 83 "cexp.y"
{ yyval.num=yyvsp[-2].num+yyvsp[0].num; ;
    break;}
case 22:
#line 84 "cexp.y"
{ yyval.num=yyvsp[-2].num+yyvsp[0].num; ;
    break;}
case 23:
#line 85 "cexp.y"
{ yyval.num=yyvsp[-2].num+yyvsp[0].num; ;
    break;}
case 24:
#line 86 "cexp.y"
{ yyval.num=yyvsp[-2].num+yyvsp[0].num; ;
    break;}
case 25:
#line 87 "cexp.y"
{ yyval.num=yyvsp[-2].num+yyvsp[0].num; ;
    break;}
case 26:
#line 88 "cexp.y"
{ yyval.num=-yyvsp[0].num; ;
    break;}
case 27:
#line 89 "cexp.y"
{ yyval.num=~yyvsp[0].num; ;
    break;}
case 28:
#line 90 "cexp.y"
{ yyval.num=yyvsp[-1].num; ;
    break;}
case 29:
#line 91 "cexp.y"
{ yyval.num=yyvsp[0].num; ;
    break;}
case 30:
#line 94 "cexp.y"
{ yyval.func=yyvsp[0].sym->val.func; ;
    break;}
case 31:
#line 98 "cexp.y"
{	yyval.num=yyvsp[-2].func(); ;
    break;}
case 32:
#line 100 "cexp.y"
{	yyval.num=yyvsp[-3].func(yyvsp[-1].num); ;
    break;}
case 33:
#line 102 "cexp.y"
{	yyval.num=yyvsp[-5].func(yyvsp[-3].num,yyvsp[-1].num); ;
    break;}
case 34:
#line 104 "cexp.y"
{	yyval.num=yyvsp[-7].func(yyvsp[-5].num,yyvsp[-3].num,yyvsp[-1].num); ;
    break;}
case 35:
#line 106 "cexp.y"
{	yyval.num=yyvsp[-9].func(yyvsp[-7].num,yyvsp[-5].num,yyvsp[-3].num,yyvsp[-1].num); ;
    break;}
case 36:
#line 108 "cexp.y"
{	yyval.num=yyvsp[-11].func(yyvsp[-9].num,yyvsp[-7].num,yyvsp[-5].num,yyvsp[-3].num,yyvsp[-1].num); ;
    break;}
case 37:
#line 110 "cexp.y"
{	yyval.num=yyvsp[-13].func(yyvsp[-11].num,yyvsp[-9].num,yyvsp[-7].num,yyvsp[-5].num,yyvsp[-3].num,yyvsp[-1].num); ;
    break;}
case 38:
#line 112 "cexp.y"
{	yyval.num=yyvsp[-15].func(yyvsp[-13].num,yyvsp[-11].num,yyvsp[-9].num,yyvsp[-7].num,yyvsp[-5].num,yyvsp[-3].num,yyvsp[-1].num); ;
    break;}
case 39:
#line 114 "cexp.y"
{	yyval.num=yyvsp[-17].func(yyvsp[-15].num,yyvsp[-13].num,yyvsp[-11].num,yyvsp[-9].num,yyvsp[-7].num,yyvsp[-5].num,yyvsp[-3].num,yyvsp[-1].num); ;
    break;}
case 40:
#line 116 "cexp.y"
{	yyval.num=yyvsp[-19].func(yyvsp[-17].num,yyvsp[-15].num,yyvsp[-13].num,yyvsp[-11].num,yyvsp[-9].num,yyvsp[-7].num,yyvsp[-5].num,yyvsp[-3].num,yyvsp[-1].num); ;
    break;}
case 41:
#line 118 "cexp.y"
{	yyval.num=yyvsp[-21].func(yyvsp[-19].num,yyvsp[-17].num,yyvsp[-15].num,yyvsp[-13].num,yyvsp[-11].num,yyvsp[-9].num,yyvsp[-7].num,yyvsp[-5].num,yyvsp[-3].num,yyvsp[-1].num); ;
    break;}
case 42:
#line 121 "cexp.y"
{ yyval.num=(yyvsp[0].num==0 ? BOOLTRUE : 0); ;
    break;}
case 43:
#line 122 "cexp.y"
{ yyval.num=(yyvsp[-2].num<yyvsp[0].num ? BOOLTRUE : 0); ;
    break;}
case 44:
#line 123 "cexp.y"
{ yyval.num=(yyvsp[-3].num<=yyvsp[0].num ? BOOLTRUE : 0); ;
    break;}
case 45:
#line 124 "cexp.y"
{ yyval.num=(yyvsp[-3].num==yyvsp[0].num ? BOOLTRUE : 0); ;
    break;}
case 46:
#line 125 "cexp.y"
{ yyval.num=(yyvsp[-3].num!=yyvsp[0].num ? BOOLTRUE : 0); ;
    break;}
case 47:
#line 126 "cexp.y"
{ yyval.num=(yyvsp[-3].num>=yyvsp[0].num ? BOOLTRUE : 0); ;
    break;}
case 48:
#line 127 "cexp.y"
{ yyval.num=(yyvsp[-2].num>yyvsp[0].num ? BOOLTRUE : 0); ;
    break;}
case 49:
#line 130 "cexp.y"
{ yyval.sym=yyvsp[0].sym; ;
    break;}
case 50:
#line 131 "cexp.y"
{ yyval.sym=yyvsp[0].sym; ;
    break;}
case 51:
#line 132 "cexp.y"
{ yyval.sym=yyvsp[0].sym; ;
    break;}
case 52:
#line 136 "cexp.y"
{ yyval.caddr=(unsigned char*)yyvsp[0].sym->val.addr; ;
    break;}
case 53:
#line 137 "cexp.y"
{ yyval.caddr=(unsigned char*)yyvsp[0].sym->val.addr; ;
    break;}
case 54:
#line 138 "cexp.y"
{ yyval.caddr=yyvsp[0].caddr; ;
    break;}
case 55:
#line 140 "cexp.y"
{ yyval.saddr=(unsigned short*)yyvsp[0].sym->val.addr; ;
    break;}
case 56:
#line 141 "cexp.y"
{ yyval.saddr=(unsigned short*)yyvsp[0].sym->val.addr; ;
    break;}
case 57:
#line 142 "cexp.y"
{ yyval.saddr=yyvsp[0].saddr; ;
    break;}
case 58:
#line 144 "cexp.y"
{ yyval.laddr=yyvsp[0].sym->val.addr; ;
    break;}
case 59:
#line 145 "cexp.y"
{ yyval.laddr=yyvsp[0].sym->val.addr; ;
    break;}
case 60:
#line 146 "cexp.y"
{ yyval.laddr=yyvsp[0].laddr; ;
    break;}
case 61:
#line 149 "cexp.y"
{ yyval.caddr=(unsigned char*)yyvsp[0].sym->val.addr; ;
    break;}
case 62:
#line 150 "cexp.y"
{ yyval.caddr=(unsigned char*)yyvsp[0].num; ;
    break;}
case 63:
#line 153 "cexp.y"
{ yyval.saddr=(unsigned short*)yyvsp[0].sym->val.addr; ;
    break;}
case 64:
#line 154 "cexp.y"
{ yyval.saddr=(unsigned short*)yyvsp[0].num; ;
    break;}
case 65:
#line 157 "cexp.y"
{ yyval.laddr=yyvsp[0].sym->val.addr; ;
    break;}
case 66:
#line 158 "cexp.y"
{ yyval.laddr=(unsigned long*)yyvsp[0].num; ;
    break;}
case 67:
#line 159 "cexp.y"
{ yyval.laddr=(unsigned long*)yyvsp[0].sym; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 163 "cexp.y"

#if 0
static unsigned long tstvar=0xdeadbeef;

static unsigned long
tstfunc(unsigned long arg)
{
	return arg;
}

static CexpSymRec tstStab[]={
	{
		"var", VAR, {addr: &tstvar }
	},
	{
		"func", FUNC, {func: tstfunc}
	},
	{
		0, 0, {0}
	},
};

static CexpSym
lookup(char *name)
{
CexpSym rval;
	for (rval=tstStab; rval->name; rval++)
		if (!strcmp(rval->name, name))
			return rval;
	return 0;
}
#endif



#define ch (*pa->chpt)
#define getch() do { (pa->chpt)++;} while(0)

int
yylex(YYSTYPE *rval, void *arg)
{
unsigned long	num;
CexpParserArg 	pa=arg;

	while (' '==ch || '\t'==ch)
		getch();

	if (isdigit(ch)) {
		/* a number */
		num=0;
		if ('0'==ch) {
			/* hex or octal */
			getch();
			if ('x'==ch) {
				/* a hex number */
				getch();
				while (isxdigit(ch)) {
					num<<=4;
					if (ch>='a')		num+=ch-'a'+10;
					else if (ch>='A')	num+=ch-'A'+10;
					else				num+=ch-'0';
					getch();
				}
			} else {
				/* OK, it's octal */
				while ('0'<=ch && ch<'8') {
					num<<=3;
					num+=ch-'0';
					getch();
				}
			}
		} else {
			/* so it must be base 10 */
			do {
				num=10*num+(ch-'0');
				getch();
			} while (isdigit(ch));
		}
		rval->num=num;
		return NUMBER;
	} else if (isalpha(ch)) {
		char idbuf[80], limit=sizeof(idbuf)-1;
		/* slurp in an identifier */
		char *chpt=idbuf;
		do {
			*(chpt++)=ch;
			getch();
		} while (isalnum(ch) && (--limit > 0));
		*chpt=0;
		/* is it one of the type cast keywords? */
		if (!strcmp(idbuf,"char"))
			return CHAR_CAST;
		else if (!strcmp(idbuf,"short"))
			return SHORT_CAST;
		else if (!strcmp(idbuf,"long"))
			return LONG_CAST;
		else if (rval->sym=cexpSymTblLookup(pa->symtbl, idbuf))
			return rval->sym->type;

		/* it's a currently undefined symbol */
		rval->id=idbuf;
		return IDENT;
	} else {
		/* it's any kind of 'special' character such as
		 * an operator etc.
		 */
		long rv=ch;
		if (rv) getch();
		/* yyparse cannot deal with '\0' chars, so we translate it back to '\n'...*/
		return rv ? rv : '\n';
	}
	return 0; /* seems to mean ERROR */
}

int
yyerror(char*msg)
{
fprintf(stderr,"Cexp syntax error: %s\n",msg);
}

int
main(int argc, char **argv)
{
int			fd;
char			*line;
CexpParserArgRec	arg={0};

if (argc<2 || (fd=open(argv[1],O_RDONLY))<0) {
	fprintf(stderr,"Need an elf symbol table file arg\n");
	return 1;
}

arg.symtbl=cexpSlurpElf(fd);

close(fd);

if (!arg.symtbl) {
	fprintf(stderr,"Error while reading symbol table\n");
	return 1;
}

while ((line=readline("Cexpr>"))) {
	arg.chpt=line;
	yyparse((void*)&arg);
	add_history(line);
	free(line);
}

cexpFreeSymTab(arg.symtbl);

#if 0
while ((sym=getsym(0,&val))>=0) {
	switch (sym) {
		default:
			fprintf(stderr,"'%c'\n",sym); break;
		case NUMBER:
			fprintf(stderr,"NUMBER: 0x%08x == %i\n",val.num,val.num); break;
		case IDENT:
			fprintf(stderr,"IDENT: '%s'\n",val.id); break;
		case CHAR_CAST:
			fprintf(stderr,"CHAR\n"); break;
		case SHORT_CAST:
			fprintf(stderr,"SHORT\n"); break;
		case LONG_CAST:
			fprintf(stderr,"LONG\n"); break;
		case VAR:
			fprintf(stderr,"VAR '%s': *(0x%08x)=0x%08x\n",
				val.sym->name, val.sym->val.addr, *val.sym->val.addr); break;
		case FUNC:
			fprintf(stderr,"FUNC '%s': (0x%08x)\n",
				val.sym->name, val.sym->val.func); break;
	}
}
#endif

return 0;
}
