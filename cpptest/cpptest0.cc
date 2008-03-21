#include <unistd.h>
#include <stdio.h>
#define RTEMS_TEST_IO_STREAM
#ifdef RTEMS_TEST_IO_STREAM
#include <iostream>
#endif

int num_inst=0;

class AClass {
public:
  AClass(const char *p = "LOCAL" ) : ptr( p )
    {
        num_inst++;
        printf(
          "%s: Hey I'm in base class constructor number %d for %p.\n",
          p, num_inst, this
        );

	/*
	 * Make sure we use some space
	 */

        string = new char[50];
	sprintf(string, "Instantiation order %d", num_inst);
    };

    virtual ~AClass()
    {
        printf(
          "%s: Hey I'm in base class destructor number %d for %p (string %p).\n",
          ptr, num_inst, this, string
        );
	print();
        num_inst--;
		delete string;
    };

    virtual void print()  { printf("%s\n", string); };

	AClass &operator=(AClass &x) { ptr=x.ptr; strcpy(string,x.string); return *this; };

protected:
    char  *string;
    const char *ptr;
private:
	AClass(AClass &);	/* string would need to be copied */
};


class BClass : public AClass {
public:
  BClass(const char *p = "LOCAL" ) : AClass( p ) 
    {
        num_inst++;
        printf(
          "%s: Hey I'm in derived class constructor number %d for %p.\n",
          p, num_inst,  this
        );

	/*
	 * Make sure we use some space
	 */

	sprintf(string, "Instantiation order %d", num_inst);
    };

    ~BClass()
    {
        printf(
          "%s: Hey I'm in derived class destructor number %d for %p.\n",
          ptr, num_inst,
          this
        );
	      print();
        num_inst--;
    };

    void print()  { printf("Derived class - %s\n", string); }
};

AClass too0 __attribute__(( init_priority(101) ))
	(   "GLOBAL    priority 101 - first   (should initialize in step 1");
AClass foo
	( "GLOBAL    default-pri  - second  (should initialize in step 6" );
BClass foobar
	( "GLOBAL    default-pri  - third   (should initialize in step 7" );
AClass too __attribute__(( init_priority(200) ))
	( "GLOBAL    priority 200 - fourth  (should initialize in step 4") ;
AClass too1 __attribute__(( init_priority(101) ))
	( "GLOBAL    priority 101 - fifth   (should initialize in step 2");
AClass toobar __attribute__(( init_priority(200) ))
	( "GLOBAL    priority 200 - sixth   (should initialize in step 5");
AClass qoobar __attribute__(( init_priority(150) ))
	( "GLOBAL    priority 110 - seventh (should initialize in step 3");

void
cdtest(void)
{
    AClass bar, blech, blah;
    BClass bleak;

#ifdef RTEMS_TEST_IO_STREAM
	printf("Starting C++ stream test:\n"); fflush(stdout);
    std::cout << "Testing a C++ I/O stream" << std::endl;
	printf("If you don't see \"Testing a C++ I/O stream\" on the previous line\n");
	printf("then this test FAILED\n"); fflush(stdout);
#else
    printf("IO Stream not tested\n");
#endif
    bar = blech;
    sleep(5);
}

void foo_function()
{
    try 
    {
      throw "foo_function() throw this exception";  
    }
    catch( const char *e )
    {
     printf( "foo_function() catch block called:\n   < %s  >\n", e );
     throw "foo_function() re-throwing execption...";  
    }
}
