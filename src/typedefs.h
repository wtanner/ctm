/*
*******************************************************************************
*
*      *******************************************************************************
*
*      File             : typedefs.h
*      Author           : EED/N/RV Peter Bloecher
*      Tested Platforms : DEC Alpha (OSF 3.2, 4.0), SUN (SunOS 5.5.1), Linux,
*                         Win95/NT
*      Description      : Definition of platform independent data
*                         types and constants
*
*      Revision history
*
*      Rev  Date       Name            Description
*      -------------------------------------------------------------------
*      pB1  1998-12-09 V.Springer      added PC Linux, PC (MSDOS/Win95/NT) tests
*                                      also for __BORLANDC__ and _MSC_VER 
*                                      compiler now
*      A    1998-05-06 P. Bloecher     first official release
*      pA2  28-MAY-97  R.Schleifer     corrected error at definitin of
*                                      maxFloat and minFloat
*      pA1  12-MAR-97  P.Bloecher      initial version
*
*
*      The following platform independent data types and corresponding
*      preprocessor (#define) constants are defined:
*
*        defined type  meaning           corresponding constants
*        ----------------------------------------------------------
*        Char          character         (none)
*        Bool          boolean           true, false
*        Byte          8-bit signed      minByte,      maxByte
*        UByte         8-bit unsigned    minUByte,     maxUByte
*        Shortint      16-bit signed     minShortint,  maxShortint
*        UShortint     16-bit unsigned   minUShortint, maxUShortint
*        Longint       32-bit signed     minLongint,   maxLongint
*        ULongint      32-bit unsigned   minULongint,  maxULongint
*        Float         floating point    minFloat,     maxFloat
*
*
*      The following compile switches are #defined:
*
*        PLATFORM      string indicating platform progam is compiled on
*                      possible values: "OSF", "PC", "SUN"
*
*        OSF           only defined if the current platform is an Alpha
*        PC            only defined if the current platform is a PC
*        SUN           only defined if the current platform is a Sun
*        
*        LSBFIRST      is defined if the byte order on this platform is
*                      "least significant byte first" -> defined on DEC Alpha
*                      and PC, undefined on Sun
*
*******************************************************************************
*/
#ifndef typedefs_h
#define typedefs_h "$Id: typedefs.h,v 1.3 1999/04/07 12:46:46 eedvsp Exp $"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/
#include <float.h>
#include <limits.h>

/*
*******************************************************************************
*                         DEFINITION OF CONSTANTS 
*******************************************************************************
*/
/*
 ********* define char type
 */
typedef char Char;

/*
 ********* define 8 bit signed/unsigned types & constants
 */
#if SCHAR_MAX == 127
typedef signed char Byte;
#define minByte  SCHAR_MIN
#define maxByte  SCHAR_MAX

typedef unsigned char UByte;
#define minUByte 0
#define maxUByte UCHAR_MAX
#else
#error cannot find 8-bit type
#endif


/*
 ********* define 16 bit signed/unsigned types & constants
 */
#if INT_MAX == 32767
typedef int Shortint;
#define minShortint     INT_MIN
#define maxShortint     INT_MAX
typedef unsigned int UShortint;
#define minUShortint    0
#define maxUShortint    UINT_MAX
#elif SHRT_MAX == 32767
typedef short Shortint;
#define minShortint     SHRT_MIN
#define maxShortint     SHRT_MAX
typedef unsigned short UShortint;
#define minUShortint    0
#define maxUShortint    USHRT_MAX
#else
#error cannot find 16-bit type
#endif


/*
 ********* define 32 bit signed/unsigned types & constants
 */
#if INT_MAX == 2147483647
typedef int Longint;
#define minLongint     INT_MIN
#define maxLongint     INT_MAX
typedef unsigned int ULongint;
#define minULongint    0
#define maxULongint    UINT_MAX
#elif LONG_MAX == 2147483647
typedef long Longint;
#define minLongint     LONG_MIN
#define maxLongint     LONG_MAX
typedef unsigned long ULongint;
#define minULongint    0
#define maxULongint    ULONG_MAX
#else
#error cannot find 32-bit type
#endif

/*
 ********* define floating point type & constants
 */
/* use "#if 0" below if Float should be double;
   use "#if 1" below if Float should be float
 */
#if 0
typedef float Float;
#define maxFloat      FLT_MAX
#define minFloat      FLT_MIN
#else
typedef double Float;
#define maxFloat      DBL_MAX
#define minFloat      DBL_MIN
#endif

/*
 ********* define complex type
 */
typedef struct {
  Float r;  /* real      part */
  Float i;  /* imaginary part */
} CPX;

/*
 ********* define boolean type
 */
typedef int Bool;
#define false 0
#define true 1

/*
 ********* Check current platform
 */
/* for Windows platforms __MSDOS__ might not be set, then it normally 
   depends on the compiler, check for Borland and Microsoft Visual C */
#if defined(__MSDOS__) || defined (__BORLANDC__) || defined (_MSC_VER)
#define PC
#define PLATFORM "PC"
#define LSBFIRST
#elif defined(__osf__)
#define OSF
#define PLATFORM "OSF"
#define LSBFIRST
#elif defined(__sun__) || defined(sparc) || defined(__sparc)
#define SUN
#define PLATFORM "SUN"
#undef LSBFIRST
#elif defined(linux) && defined(i386)
/* Linux could be installed on any other computer as well, take care of 
   PC installation here */
#define PCLINUX
#define PLATFORM "PCLINUX"
#define LSBFIRST
#else
#error "can't determine architecture; adapt typedefs.h to your platform"
#endif

#endif
