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
extern int run_cpp_test(void);
}

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
void foo_function(int *);



int run_cpp_test(void)
{
int rval = 0;

    printf( "\n\n*** CONSTRUCTOR/DESTRUCTOR TEST ***\n" );

    cdtest();

    printf( "*** END OF CONSTRUCTOR/DESTRUCTOR TEST ***\n\n\n" );


    printf( "*** TESTING C++ EXCEPTIONS ***\n\n" );

    try 
    {
      foo_function( &rval );
	  fprintf(stderr,"FAILED: - execution flow not changed (char* exception should have been thrown)\n");
	  rval++;
    }
    catch( const char *e )
    {
       printf( "Success catching a char * exception\n%s\n", e );
    }
	catch(...)
	{
	  fprintf(stderr,"FAILED: - foo_function() threw an unexpected exception\n");
	  rval++;
	}

    try 
    {
      printf( "throw an instance based exception\n" );
	  throw RtemsException( __FILE__, __LINE__, 0x55 ); 
	  fprintf(stderr,"FAILED: - execution flow not changed (RtemsException should have been thrown)\n");
	  rval++;
    }
    catch( RtemsException & ex ) 
    {
       printf( "Success catching RtemsException...\n" );
       ex.show();
    }
    catch(...) 
    {
      printf( "FAILED: Caught another (unexpected) exception.\n" );
	  rval++;
    }
	if ( 0 == rval )
    	fprintf(stderr, "PASSED: Exceptions are working properly.\n" );
	else 
		fprintf(stderr, "FAILURE: %i errors encountered\n", rval);

    sleep(2);

    printf( "Global Dtors should be called after unloading the 'ctdt' module....\n" );

	return rval;
}
