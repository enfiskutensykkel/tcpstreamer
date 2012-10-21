#ifndef __DEBUG__
#define __DEBUG__
#if defined(DEBUG) && !defined(NDEBUG)
/* Verbose debugging turned on */

#include <stdio.h>
#include <errno.h>
#include <string.h>

/* Ensure portability */
#ifdef assert
#undef assert
#endif


/* Evaluate scalar expression */
#define assert(expr) \
	do{if(!(expr)){fprintf(stderr,"[%s %s:%u] Assertion failed: %s\n",__FILE__,__func__,__LINE__,#expr);while(1);}}while(0)

/* Output a scalar expression and what it evaluates to */
#define dbgexp(expr, frmt) \
	do{fprintf(stderr,"[%s %s:%u] %s ==> " frmt "\n",__FILE__,__func__,__LINE__,#expr,expr);}while(0)

/* Output a custom formatted debug string */
#define dbgout(frmt, ...) \
	do{fprintf(stderr,"[%s %s:%u] " frmt "\n",__FILE__,__func__,__LINE__,__VA_ARGS__);}while(0)

/* Check if an error code was set */
#define dbgchk(code) \
	do{if(errno==(code)){fprintf(stderr,"[%s %s:%u] %s is set: %s\n",__FILE__,__func__,__LINE__,#code,strerror(errno));}}while(0)

/* Output errno */
#define dbgerr(arg) \
	do{fprintf(stderr,"[%s %s:%u] errno is %d: %s\n",__FILE__,__func__,__LINE__,errno,strerror(errno));}while(0)


#else
/* Verbose outputting turned off */

#include <stdio.h>
#include <errno.h>
#include <string.h>

/* Ensure portability */
#ifdef assert
#undef assert
#endif

#define dbgexp(expr, frmt)
#define dbgout(frmt, ...)
#define assert(expr) do{if(!(expr))fprintf(stderr,"Unexpected value: %s\n",#expr);}while(0)
#define dbgchk(code) do{if(errno==(code))fprintf(stderr,"Unexpected error: %d\n",code);}while(0)
#define dbgerr(arg) do{fprintf(stderr,"Unexpected error: %s\n",(arg==NULL?strerror(errno):arg));}while(0)

#endif
#endif
