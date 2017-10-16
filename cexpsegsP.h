/* private interface to cexp memory segment */

/* SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 *
 * Authorship
 * ----------
 * This software (CEXP - C-expression interpreter and runtime
 * object loader/linker) was created by
 *
 *    Till Straumann <strauman@slac.stanford.edu>, 2002-2008,
 * 	  Stanford Linear Accelerator Center, Stanford University.
 *
 * Acknowledgement of sponsorship
 * ------------------------------
 * This software was produced by
 *     the Stanford Linear Accelerator Center, Stanford University,
 * 	   under Contract DE-AC03-76SFO0515 with the Department of Energy.
 * 
 * Government disclaimer of liability
 * ----------------------------------
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied, or
 * assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately owned
 * rights.
 * 
 * Stanford disclaimer of liability
 * --------------------------------
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * Stanford disclaimer of copyright
 * --------------------------------
 * Stanford University, owner of the copyright, hereby disclaims its
 * copyright and all other rights in this software.  Hence, anyone may
 * freely use it for any purpose without restriction.  
 * 
 * Maintenance of notices
 * ----------------------
 * In the interest of clarity regarding the origin and status of this
 * SLAC software, this and all the preceding Stanford University notices
 * are to remain affixed to any copy or derivative of this software made
 * or distributed by the recipient and are to be affixed to any copy of
 * software made or distributed by the recipient that contains a copy or
 * derivative of this software.
 * 
 * SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 

#ifndef CEXP_CEXPSEGS_P_H
#define CEXP_CEXPSEGS_P_H

#define SEG_ATTR_EXEC	(1<<0)
#define SEG_ATTR_RO     (1<<1)

typedef struct CexpSegmentRec_ *CexpSegment;

/*
 * Allocate CexpSegmentRecs (no real memory
 * is allocated yet) as appropriate for
 * target architecture.
 *
 * RETURNS: number of segments, pointer to
 *          array of CexpSegmentRecs in *ptr.
 *
 *          NOTE: the list of SegmentRecs is
 *          terminated by a sentinel with a
 *          NULL 'name' member. (Sentinel
 *          is not counted with the return
 *          value.)
 *
 *          On error, a number less than 1 
 *          is returned and *ptr is NULL.
 */
int cexpSegsInit(CexpSegment *ptr);

/*
 * Allocate memory for the CexpSegmentRec array
 * only (for use by target-specific cexpSegsInit)
 */
CexpSegment cexpSegsCreate(int n);


/*
 * Allocate memory for all the segments themselves
 * and attach to 'chunk' pointers.
 */

int cexpSegsAllocAll(CexpSegment ptr);

/*
 * Release all CexpSegmentRecs and attached
 * memory.
 */
void cexpSegsDeleteAll(CexpSegment  ptr);

/*
 * Allocate memory for 'this' segment and attach to 'chunk'
 * (returns error if alreay populated; must release first)
 */
int cexpSegsAllocOne(CexpSegment ptr);

/*
 * Release memory for 'this' segment and set 'chunk' to 0
 */
void cexpSegsDeleteOne(CexpSegment  ptr);

struct bfd;
struct sec;

/*
 * Find segment for a given section.
 * RETURNS: segment or NULL on error.
 */
CexpSegment cexpSegsMatch(CexpSegment segArray, struct bfd *abfd, void *s);


typedef enum {
	CEXP_SEG_TEXT,
	CEXP_SEG_DATA,
	CEXP_SEG_VENR,
	CEXP_SEG_END
} CexpSegType;

CexpSegment cexpSegsGet(CexpSegment segArray, CexpSegType type);

/* A segment of output memory */
typedef struct CexpSegmentRec_ {
	void             *chunk;      /* pointer to actual memory    */
	unsigned long    vmacalc;     /* working counter for linker  */
	unsigned long    size;
	unsigned long    attributes;  /* such as 'read-only', 'exec' */
	const char       *name;       /* info                        */
	int              (*allocat)(CexpSegment);
	void             (*release)(CexpSegment);
	void             *pvt;        /* for use by allocat/release  */
} CexpSegmentRec;

#endif
