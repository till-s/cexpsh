/* $Id$ */

/* Support for tecla features (such as word completion etc.) */

/* LICENSING INFORMATION: note that the licensing terms for
 * using libtecla may be different from the EPICS open license
 * who applies to this file
 */

/*
 * Copyright 2002, Stanford University and
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

#include <libtecla.h>
#include <cexp_regex.h>

#include "cexpsyms.h"
#include "cexpmod.h"
#include "ctyps.h"

#define	 MATCH_MAX	100

/* ugly hack - this must match the definition in cexp.y */

/* if the lexer detects an unterminated string constant
 * it returns LEXERR_INCOMPLETE_STRING - offset;
 * the offset is the difference between the current position
 * ( == end of the string) and the opening quote.
 */
#define LEXERR_INCOMPLETE_STRING	(-100)
extern int		cexplex();

extern CexpSym	_cexpSymLookupRegex();

int
cexpSymComplete(WordCompletion *cpl, void *closure, const char *line, int word_end)
{
int				rval=1;
int 			word_start;
cexp_regex		*rc=0;
char			*pattern=0;
int				count=MATCH_MAX,i;
CexpSym			s;
CexpModule		m;
CexpParserCtx	ctx = closure;
CexpTypedValRec	dummy;
int				quote;

	cexpResetParserCtx(ctx, line);
	/* try to find an opening quote using the lexer
	 * it returns a magic error code containing the
	 * offset (with respect to the end of the line)
	 * of such an opening quote...
	 */
	while ( (quote=cexplex(&dummy, ctx)) > 0 && '\n' != quote )
		/* nothing else to do */;

	if ( quote <= LEXERR_INCOMPLETE_STRING ) {
		int			rval;
		CplFileConf	*conf = new_CplFileConf();

		/* start position = end + offset returned by cexplex() */
		cfc_file_start(conf, word_end + quote - LEXERR_INCOMPLETE_STRING);
		rval = cpl_file_completions(cpl, conf, line, word_end);
		del_CplFileConf(conf);
		return rval;
	}

	/* search start of the word */
	for (word_start=word_end; word_start>0; word_start--) {
		/* these characters should match the lexer, see cexp.y ISIDENTCHAR() */
		register int ch=(unsigned char)line[word_start-1];
		if (! (isalpha(ch) || '_'==ch || '@'==ch) )
			break;
	}
	if (word_start<word_end) {
		pattern=calloc(word_end-word_start+5,1);
		pattern[0]='^';
		strncpy(pattern+1, line+word_start, word_end-word_start);
		rc=cexp_regcomp(pattern);
		/* the lookup routine returns the last symbol found
		 * looping for 'too_many' instances. Hence, if it still
		 * finds something after too_many matches, we reject...
		 */
		_cexpSymLookupRegex(rc,&count,0,0,0);
	} else {
		count=0;
	}
	if (count<=0) {
		cpl_record_error(cpl,"Refuse to complete: too many matches");
		goto cleanup;
	}

	/* now, the real fun starts.. 
	 *
	 * NOTE the race condition: if another user just loaded 
	 * a huge module, we might still have too many matches here.
	 * However, the damage is minimal: we just don't present
	 * all the choices...
	 */
	for (s=0,m=0,count=0,i=1; count<MATCH_MAX && (s=_cexpSymLookupRegex(rc,&i,s,0,&m)); i=1,count++) {
		cpl_add_completion(	cpl, line, word_start, word_end,
							s->name + word_end-word_start,	
							CEXP_TYPE_FUNQ(s->value.type) ? "()" : "",
							CEXP_TYPE_FUNQ(s->value.type) ? "("  : "");
	}

	rval=0;

cleanup:
	cexp_regfree(rc);
	free(pattern);
	return rval;
}
