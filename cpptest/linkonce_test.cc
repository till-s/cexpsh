#include <stdio.h>

#ifndef LINKONCE_INST
#define LINKONCE_INST 0
#endif

// virtual class members and vtable go into linkonce sections
class test_linkonce {
public:
	test_linkonce() : inst(LINKONCE_INST) { }
	virtual int exc_test(int arg) {
		if ( arg )
			throw(LINKONCE_INST);
		return LINKONCE_INST;
	}
	virtual int lo_test(int arg) {
		static int the_inst = LINKONCE_INST; /* goes into linkonce 'data' section */
		if ( arg )
			return the_inst;
		return LINKONCE_INST;
	}
private:
	int inst;
};

#define __CC(x,y,z)   x##y##z
#define DECL_OBJ(x,z) __CC(lot,x,z)
#define OBJNM(x)      DECL_OBJ(LINKONCE_INST,x)

#if LINKONCE_INST > 0 && LINKONCE_INST < 10
test_linkonce OBJNM(_s);
test_linkonce *OBJNM() = &OBJNM(_s);
#else

#include <sstream>
#include <stdexcept>
#include <string.h>

// access external objects through pointers -- otherwise
// virtual member functions may be inlined here...
extern test_linkonce *lot1;
extern test_linkonce *lot2;

#define NAM "linkonce_test "

test_linkonce me;

/* This emits a linkonce section that should
 * be already present in the main executable.
 * Luciano Piccoli reported a problem with 
 * cexp not loading a module because the
 * section symbol of a discarded linkonce
 * section couldn't be found.
 * This was due to a relocation in the .eh_frame.
 * Relocations in the .eh_frame referring to
 * discarded linkonces should be ignored!
 */
static const char *string_gen(const char *s)
{
std::stringstream ss(s);
// ss << "This is yet" << " an";
// ss << "other " << "test";
 return strdup(ss.str().c_str());
 //cout << ss.str() << endl;
}

extern "C" int run_linkonce_test(void)
{
int v;
int rval = 0;
const char *s1, *s2;

	// Verify that both instances return the same number (virtual member code loaded with instance 1)
	if ( 1 != (v = lot1->exc_test(0)) ) {
		fprintf(stderr,NAM"FAILED: lot1->exc_test(0) should return 1 -- but got %i\n", v);
		rval++;
	}
	if ( 1 != (v = lot2->exc_test(0)) ) {
		fprintf(stderr,NAM"FAILED: lot2->exc_test(0) should return 1 (virtual member from 1st instance) -- but got %i\n", v);
		rval++;
	}
	if ( 1 != (v = lot1->lo_test(0)) ) {
		fprintf(stderr,NAM"FAILED: lot1->lo_test(0) should return 1 -- but got %i\n", v);
		rval++;
	}
	if ( 1 != (v = lot2->lo_test(0)) ) {
		fprintf(stderr,NAM"FAILED: lot2->lo_test(0) should return 1 (virtual member from 1st instance) -- but got %i\n", v);
		rval++;
	}
	// Verify that local instance uses inlined member
	if ( LINKONCE_INST != (v = me.lo_test(0)) ) {
		fprintf(stderr,NAM"FAILED: me.lo_test(0) should return %i (inlined virtual member) -- but got %i\n", LINKONCE_INST, v);
		rval++;
	}

	// Verify that both instances retrieve the same number from linkonce data section
    // (however, using the linkonce virtual function anyways so that should get the
	// first data instance no matter what).
	if (    1 != (v = lot1->lo_test(1)) ) {
		fprintf(stderr,NAM"FAILED: lot1.lo_test(1) should return 1 -- but got\n", v);
		rval++;
	}
	if (    1 != (v = lot2->lo_test(1)) ) {
		fprintf(stderr,NAM"FAILED: lot2.lo_test(1) should return 1 (virtual member + data from 1st instance) -- but got\n", v);
		rval++;
	}

	// Verify that me.lo_test() also retrieves the same number from linkonce
	// data section. Since 'me' is defined locally the 'lo_test' member
	// is probably in-lined but should still refer to the linkonce data section
	if (    1 != (v = me.lo_test(1)) ) {
		fprintf(stderr,NAM"FAILED: me.lo_test(1) should return 1 (inlined virtual member + data from 1st instance) -- but got\n", v);
		rval++;
	}

	// Verify that the first instance exception is thrown
	try {
		lot1->exc_test(1);
		fprintf(stderr,NAM"FAILED lot1->exc_test(1) should throw and exception\n");
		rval++;
	} catch (int e) {
		if ( 1 != e ) {
			fprintf(stderr,NAM"FAILED lot1->exc_test(1) threw exception %i (instead of 1)\n", e);
			rval++;
		}
	} catch (...) {
		fprintf(stderr,NAM"FAILED lot1->exc_test(1) threw unknown exception!\n");
		rval++;
	}
	
	try {
		lot2->exc_test(1);
		fprintf(stderr,NAM"FAILED lot2->exc_test(1) should throw and exception\n");
		rval++;
	} catch (int e) {
		if ( 1 != e ) {
			fprintf(stderr,NAM"FAILED lot2->exc_test(1) threw exception %i (instead of 1)\n", e);
			rval++;
		}
	} catch (...) {
		fprintf(stderr,NAM"FAILED lot2->exc_test(1) threw unknown exception!\n");
		rval++;
	}

	try {
		me.exc_test(1);
		fprintf(stderr,NAM"FAILED me.exc_test(1) should throw and exception\n");
		rval++;
	} catch (int e) {
		if ( LINKONCE_INST != e ) {
			fprintf(stderr,NAM"FAILED me.exc_test(1) threw exception %i (instead of %i)\n", e, LINKONCE_INST);
			rval++;
		}
	} catch (...) {
		fprintf(stderr,NAM"FAILED me.exc_test(1) threw unknown exception!\n");
		rval++;
	}

	// verify that stringstream throws an exception 
	try {
		s1 = string_gen(0);
		fprintf(stderr,NAM"FAILED string_gen(NULL) should throw an exception\n");
		rval++;
	} catch(std::logic_error e) {
		// OK, that's expected
	} catch (...) {
		fprintf(stderr,NAM"FAILED string_gen(NULL) threw unknown exception!\n");
		rval++;
	}

	s2 = "hello";
	s1 = string_gen(s2);
	// strcmp is jumped to via PLT on modern linux (demo system) to pick cpu-optimized
	// variant. Avoid by using our own implementation :-(
	for ( v = 0; s1[v] == s2[v] && s1[v]; v++ ) 
		;
	if ( s1[v] != s2[v] ) {
		fprintf(stderr,NAM"FAILED string_gen(%s) returned: '%s' (%i)\n", s2, s1, strcmp(s1,s2));
		rval++;
	}
	delete s1;
		
	if ( 0 == rval )
		fprintf(stderr,NAM"PASSED!\n");

	return rval;
}


#ifdef DO_MAIN
int
main()
{
	run_linkonce_test();
}
#endif
#endif

