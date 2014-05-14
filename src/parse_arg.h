/*
*******************************************************************************
*
*      *******************************************************************************
*
*      File             : parse_arg.h
*      Author           : Matthias Doerbecker
*      Tested Platforms : Sun Solaris, Windows NT
*      Description      : Functions for parsing the argument line
*
*      Revision history
*
*      Rev  Date         Name            Description
*      -------------------------------------------------------------------
*      pA1  03-JAN-2000  M. Doerbecker   initial version
*
*******************************************************************************
*/
#ifndef parse_arg
#define parse_arg "$Id: $"


/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include <stdarg.h>

/*
*******************************************************************************
*                         DEFINITION OF CONSTANTS
*******************************************************************************
*/

/*
*******************************************************************************
*                         DEFINITION OF DATA TYPES
*******************************************************************************
*/



/*
*******************************************************************************
*                         DECLARATION OF PROTOTYPES
*******************************************************************************
*/


/*
*******************************************************************************
*
*     Function        : get_option
*     In              : argc            same as in main(...)
*                     : argv            same as in main(...)
*                     : num_opts        number of options that are following in
*                                       the variable argument list
*                     : ...             list of valid options, each option is 
*                                       specified by a string variable, the 
*                                       list must contain num_opts strings.
*                     : arg_index       acutal argument index
*     In/Out          : *ptr_arg_index  acutal argument (is increased after an
*                                       an option has been recognized)
*     Calls           : va_start(), va_arg(), va_end()
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : index of the option (between 0 and num_opts-1) 
*                       on success, -1 if option is unknown
*     Information     : check whether an argument is a valid option
*
*******************************************************************************
*/


int get_option(int argc, const char **argv, int arg_index, int num_opts, ...);



/*
*******************************************************************************
*
*     Function        : get_argument
*     In              : argc            same as in main(...)
*                     : argv            same as in main(...)
*                     : arg_index       acutal argument index
*     Out             : <none>
*     Calls           : <none>
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : argument string, (char*)NULL if all arguments have
*                       already been consumed
*     Information     : get the next argument of the command line
*
*******************************************************************************
*/

const char* get_argument(int argc, const char **argv, int arg_index);

#endif
