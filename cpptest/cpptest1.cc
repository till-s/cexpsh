/*
 *  This routine is the initialization task for this test program.
 *  It is called from init_exec and has the responsibility for creating
 *  and starting the tasks that make up the test.  If the time of day
 *  clock is required for the test, it should also be set to a known
 *  value by this function.
 *
 *  Input parameters:  NONE
 *
 *  Output parameters:  NONE
 *
 *  COPYRIGHT (c) 1994 by Division Incorporated
 *  Based in part on OAR works.
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *
 *  by Rosimildo da Silva:
 *  Modified the test a bit to indicate when an instance is
 *  global or not, and added code to test C++ exception.
 *
 *
 *  main.cc,v 1.13 2001/11/26 14:33:43 joel Exp
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" 
{
extern void cpptestStart(void);
}

extern int num_inst;

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
  BClass(const char *p = "LOCAL" );
  ~BClass();
  void print();
};

class RtemsException 
{
public:
    
    RtemsException( const char *module, int ln, int err = 0 )
    : error( err ), line( ln ), file( module )
    {
      printf( "RtemsException raised=File:%s, Line:%d, Error=%X\n",
               file, line, error ); 
    }

    void show()
    {
      printf( "RtemsException ---> File:%s, Line:%d, Error=%X\n",
               file, line, error ); 
    }

private:
   int  error;
   int  line;
   const char *file;

};

void cdtest(void);
void foo_function();



void cpptestStart(void)
{
    printf( "\n\n*** CONSTRUCTOR/DESTRUCTOR TEST ***\n" );

    cdtest();

    printf( "*** END OF CONSTRUCTOR/DESTRUCTOR TEST ***\n\n\n" );


    printf( "*** TESTING C++ EXCEPTIONS ***\n\n" );

    try 
    {
      foo_function();
    }
    catch( const char *e )
    {
       printf( "Success catching a char * exception\n%s\n", e );
    }
    try 
    {
      printf( "throw an instance based exception\n" );
		throw RtemsException( __FILE__, __LINE__, 0x55 ); 
    }
    catch( RtemsException & ex ) 
    {
       printf( "Success catching RtemsException...\n" );
       ex.show();
    }
    catch(...) 
    {
      printf( "Caught another exception.\n" );
    }
    printf( "Exceptions are working properly.\n" );
    sleep(5);
    printf( "Global Dtors should be called after unloading the cpptest0 module....\n" );
}
