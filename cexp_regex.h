#ifndef CEXP_REGEX_H
#define CEXP_REGEX_H
/* $Id$ */

/* Portability wrapper for different regex implementations */

/*
 * Copyright 2003, Stanford University and
 * 		Till Straumann <strauman@slac.stanford.edu>
 * 
 * Stanford Notice
 * ***************
 * 
 * Acknowledgement of sponsorship
 * * * * * * * * * * * * * * * * *
 * This software was produced by the Stanford Linear Accelerator Center,
 * Stanford University, under Contract DE-AC03-76SFO0515 with the Department
 * of Energy.
 * 
 * Government disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied,
 * or assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately
 * owned rights.
 * 
 * Stanford disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * This product is subject to the EPICS open license
 * - - - - - - - - - - - - - - - - - - - - - - - - - 
 * Consult the LICENSE file or http://www.aps.anl.gov/epics/license/open.php
 * for more information.
 * 
 * Maintenance of notice
 * - - - - - - - - - - -
 * In the interest of clarity regarding the origin and status of this
 * software, Stanford University requests that any recipient of it maintain
 * this notice affixed to any distribution by the recipient that contains a
 * copy or derivative of this software.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* BUMMER: libiberty/regex is NOT reentrant */
#define HAVE_SPENCER_REGEX
#undef  HAVE_LIBIBERTY

#if 	defined(HAVE_LIBIBERTY)
#elif   defined(HAVE_SPENCER_REGEX)
#include <spencer_regexp.h>

typedef SPENCER_(regexp) cexp_regex;

#define cexp_regcomp(expr)		SPENCER_(regcomp)(expr)
#define cexp_regexec(ex,str)	SPENCER_(regexec)(ex,str)

#include <stdlib.h>

#define cexp_regfree(regex) free(regex)

#endif

#endif
