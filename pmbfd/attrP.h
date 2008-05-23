/* $Id$ */

/* 
 * Authorship
 * ----------
 * This software ('pmelf' ELF file reader) was created by
 *     Till Straumann <strauman@slac.stanford.edu>, 2008,
 * 	   Stanford Linear Accelerator Center, Stanford University.
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
 * ------------------ SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 
#ifndef PMELF_ATTR_PRIVATE_H
#define PMELF_ATTR_PRIVATE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pmelf.h>
#include <stdio.h>

#define FORMAT_VERSION_A 'A'

/* Top-level tags */
#define Tag_File          1
#define Tag_Section       2
#define Tag_Symbol        3
/* Publicly known 'sub-tags' */
#define Tag_Compat        32

/*
 * Convention for 'public' attributes; if LSB is
 * set the value is a (uleb128 encoded) integer
 * and a NUL-terminated string otherwise.
 */
#define TAG_NUMERICAL( tag ) ( ! ((tag) & 1) )

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Representation types of 'public' tag values
 */
typedef enum {
	Pmelf_Attribute_Type_unknown = -1,
	Pmelf_Attribute_Type_none,
	Pmelf_Attribute_Type_num,
	Pmelf_Attribute_Type_str,
	Pmelf_Attribute_Type_both
} Pmelf_pub_attribute_t;

/*
 * Attribute value in 'public' representation
 */

typedef struct Pmelf_pub_attribute_ {
	Elf32_Word	i;
	const char *s;
} Pmelf_pub_attribute;

/*
 * Representation of tag/attribute values are
 * not strictly defined. Vendors are free to
 * use a private representation...
 */
typedef uint8_t Pmelf_opaque_attribute[];

/*
 * Tag-Value pair which can be on a linked-list
 */
typedef struct Pmelf_attribute_list_ {
	struct Pmelf_attribute_list_ *next;
	union {
		struct {
			int                 tag;
			Pmelf_pub_attribute val;
		}                         pub;
		struct {
			int                 tag;
			uint8_t             val[];
		}                         opq;
	}                             att;
} Pmelf_attribute_list;

/*
 * Table of attributes associated with a
 * particular 'vendor'.
 */
typedef struct {
	struct Pmelf_attribute_set_    *aset;       /* pointer back to the container            */
	struct Pmelf_attribute_vendor_ *pv;         /* vendor knowing how to handle this set    */
	union   {
		Pmelf_opaque_attribute *p_opq;
		Pmelf_pub_attribute    *p_pub;
	}                               vals;       /* pointer to array of values belonging to
	                                             * tags registered in 'map'
	                                             */
	uint8_t	                        avail;      /* # of free units in 'vals' array          */
	uint8_t	                        idx;        /* index of next available unit in 'vals'   */
	Pmelf_attribute_list            *lst;       /* linked-list of other tag/value pairs
	                                             * (which are not in the 'vals' array)
	                                             */
	uint8_t	                        map[];      /* map 'tag' into an index for 'vals'       */
} Pmelf_attribute_tbl;

/*
 * 'Vendor' access methods 
 */
struct Pmelf_attribute_vendor_ {
	struct Pmelf_attribute_vendor_ *next;
	const char                     *name;
	                               /* Read attributes from memory buffer 'buf' of size 'size' into an attribute
									* table (table is provided by caller and may or may not be empty, i.e.,
									* the routine may have to add to the existing attributes already
									* stored in the table).
									* The 'file_attributes_read' routine is responsible for allocating
									* memory where the actual attributes are stored (i.e., the 'vals'
									* array and/or 'lst' list in the table).
									* RETURN: 0 on success, nonzero on error
									*/
	int                            (*file_attributes_read)(Pmelf_attribute_tbl *patbl, const uint8_t *buf, unsigned size);
	                               /* Release resources attached to the table (by read routine)
									* (Note: the table itself is owned by the caller and must not be free()ed)
									*/
	void                           (*file_attributes_destroy)(Pmelf_attribute_tbl *patbl);
	                               /* Check for compatibility of attributes stored in tables patbl and pbtbl.
									* RETURN: 0 if no incompatibility is found, negative value if the sets
									*         describe incompatible features.
									*/
	int                            (*file_attributes_match)(Pmelf_attribute_tbl *patbl,  Pmelf_attribute_tbl *pbtbl);
	                               /* Return representation type of value associated with 'tag'
									* (If attributes are not of a 'public' representation then
									* meaning of the return values are private to this
									* 'vendor'/implementation
									*/
	Pmelf_pub_attribute_t          (*file_attributes_tag_type)(Pmelf_attribute_tbl *patbl, Elf32_Word tag);
	                               /* Return a pointer to a static string describing 'tag'; used for
									* informational purposes. This routine is optional and the
									* function pointer may be NULL.
									*/
	const char * const             (*file_attributes_tag_name)(Pmelf_attribute_tbl *patbl, Elf32_Word tag);
	                               /* Dump attribute table to FILE. This routine is optional
									* and may be NULL.
									*/
	void                           (*file_attributes_print)(Pmelf_attribute_tbl *patbl, FILE *f);
	                               /* Dump a single attribute to FILE.
									* This routine is called by the 'file_attributes_print' method
									* which defines the meaning/representation of the 'attribute'
									* argument.
									* This function pointer exists so that a more general
									* 'file_attributes_print' routine can be shared among 'vendors'
									* but still produce vendor-specific details (by calling
									* this hook).
									* RETURN: number of characters printed.
									*/
	int                            (*file_attribute_print) (Pmelf_attribute_tbl *patbl, FILE *f, Elf32_Word tag, void *attribute);
	int                            /* last tag in the 'vals' array; tags > max_tag are found in the linked list.
	                                * FIXME: maybe we should do away with the array altogether...
	                                */
	                               max_tag;
};

#define ATTR_MAX_VENDORS 5

/*
 * An attribute set. Can consist of multiple
 * tables, each associated with a particular
 * vendor who knows how to manipulate/print/
 * compare them.
 * However: all attributes belonging to a 
 *          particular vendor are in the
 *          same table.
 */
struct Pmelf_attribute_set_ {
	struct {
		Pmelf_attribute_tbl    *file_attributes;
	}                      attributes[ATTR_MAX_VENDORS];
	/* section and symbol attributes not yet supported */
	void                   *scn_data;	/* keep section data around; contains all the strings for now */
	char                   *obj_name;
};

/******************************************************************/
/**** GENERIC 'METHODS' TO BE USED BY VENDOR IMPLEMENTATIONS  *****/
/******************************************************************/

/* Generic 'file_attributes_read' function which knows
 * how to parse a byte stream with all attribute values
 * being either uleb128-encoded numbers or NUL-terminated
 * strings.
 * Decodes a stream of format
 *
 * (entities with
 *     -u32 suffix are 32-bit numbers of host endian-ness
 *     -str suffix are NUL-terminated ascii-character strings
 *     -ulb suffix are uleb128-encoded numbers of max. 32-bit.)
 *
 * stream = 
 *
 * 'A', [ <subsection-length-u32>  <vendor-name-str>
 *         [    <file-tag-ulb> <sub-subsection-length-u32> <pub-attribute> * 
 *           |  <sect-tag-ulb> <sub-subsection-length-u32> <pub-attribute> *
 *           |  <symb-tag-ulb> <sub-subsection-length-u32> <pub-attribute> *
 *         ] +
 *       ] *
 *
 * with
 *
 * pub-attribute = <tag-ulb> [ <val-ulb> ] [ <val-str> ]
 *
 * Which kind/type of value is to be expected for a particular tag is announced
 * by the 'file_attributes_tag_t' method. By convention, odd-numbered tags
 * are numerical, even-numbered tags are strings (but e.g., the even-numbered,
 * public 'compatibility-tag' (#32) uses two values, a number ['flags'] AND a
 * string ['vendor' associated with 'flags'].
 *
 * RETURNS: 0 on success, nonzero on error.
 */
int
pmelf_pub_file_attributes_read(Pmelf_attribute_tbl *patbl, const uint8_t *buf, unsigned size);

/*
 * Destroy contents of attribute table which was populated by
 * pmelf_pub_file_attributes_read().
 * Use this routine as a companion to pmelf_pub_file_attributes_read().
 */
void 
pmelf_pub_file_attributes_destroy(Pmelf_attribute_tbl *patbl);

/*
 * Match all Tag_Compat attributes found in tables 'patbl' and 'pbtbl'
 *
 * RETURN: zero if no incompatibility was found, nonzero otherwise.
 */
int
pmelf_pub_file_attributes_match_compat(Pmelf_attribute_tbl *patbl, Pmelf_attribute_tbl *pbtbl);

/*
 * Print an attribute table in 'public' representation (i.e, one that
 * can be parsed by pmelf_pub_file_attributes_read()).
 *
 * Tags/values with vendor-specific meaning can be dumped by a
 * vendor-specific file_attribute_print() routine (which can e.g.,
 * print a meaningful string instead of just the tag number).
 */
void
pmelf_pub_file_attributes_print(Pmelf_attribute_tbl *patbl, FILE *f);

/*
 * Generic routine to print an attribute in 'public' representation w/o any deeper knowledge
 * about what it means, i.e., output is basically numerical.
 *
 * RETURNS: number of chars printed.
 */
int
pmelf_pub_file_attribute_print(Pmelf_attribute_tbl *patbl, FILE *f, Elf32_Word tag, void *att);

/* Decode 'uleb128'-encoded number from memory
 * buffer (of size 'sz').
 *
 * RETURNS: Number of bytes read or -1 on failure
 *          (attempt to read beyond buffer size).
 *
 *          Decoded number is deposited in *p.
 *
 * NOTE:    Decoded number is only 32-bits but
 *          the encoded number could be longer
 *          (if buffer is not exhausted).
 *          Only the least-significant 32-bits
 *          are deposited in '*p' in this case.
 */
int
pmelf_guleb128(const uint8_t *s, Elf32_Word *p, int sz);

/*
 * Print string description of 'tag' (if file_attributes_tag_name method
 * is available and knows about the tag). Otherwise the tag is printed
 * in numerical form.
 *
 * RETURNS: numbers of characters printed.
 */
int
pmelf_pub_print_tag(Pmelf_attribute_tbl *patbl, FILE *f, int tag);

/*
 * Print single attribute (if file_attribute_print method is available)
 *
 * RETURNS: number of characters printed (zero if no print method available).
 *
 * NOTE: representation of 'att' must be as expected by the vendor
 *       file_attribute_print() method.
 */
int
pmelf_print_attribute(Pmelf_attribute_tbl *patbl, FILE *f, Elf32_Word tag, void *att);

/*
 * Read 32-bit word and byte-swap if requested (caller must know whether
 * this is needed).
 *
 * RETURNS: 0 on success, nonzero on error (attempt to read beyond
 *          buffer boundary, no byte-swapping configured).
 *
 *          On success, the value is deposited in *pval.
 *
 * NOTE   : pass name of caller in 'nm' for providing info used by
 *          error message.
 */
int
pmelf_getword(int needswap, const uint8_t *b, int sz, Elf32_Word *pval, char *nm);

#ifdef __cplusplus
};
#endif

#endif

