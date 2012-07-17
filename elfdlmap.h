#ifndef CEXP_DLMAP_H
#define CEXP_DLMAP_H

#include <stdint.h>

/* build a link map reflecting the current executable
 * and all shared libraries it depends on.
 */

typedef struct CexpLinkMapRec_ *CexpLinkMap;

/* If string table is 'static' */
#define CEXP_LINK_MAP_STATIC_STRINGS (1<<0)

typedef struct CexpLinkMapRec_ {
	CexpLinkMap   next;      /* linked list                              */
	void          *elfsyms;
	const char    *strtab;
	const char    *name;     /* optional; may be empty                   */
	unsigned long nsyms;
	unsigned long firstsym;  /* if there is a gnu hashtable then         */
                             /* all undefined or local symbols are first */
	uintptr_t     offset;    /* offset by which symbol values may need   */
	                         /* to be adjusted.                          */
	int           flags;
} CexpLinkMapRec;

/* Build a link map
 * 'name' is currently unused and must be NULL
 * to select the currently executing programm
 */
CexpLinkMap
cexpLinkMapBuild(const char *name, void *parm);

void
cexpLinkMapFree(CexpLinkMap m);

#endif
