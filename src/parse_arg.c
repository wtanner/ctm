/*
*******************************************************************************
*
*      *******************************************************************************
*
*      File             : parse_arg.c
*      Author           : Matthias Doerbecker
*      Tested Platforms : Sun Solaris, Windows NT
*      Description      : Functions for parsing the argument line
*
*      Revision history
*
*      Rev  Date         Name            Description
*      -------------------------------------------------------------------
*      pA1  03-JAN-2000  M.Doerbecker    initial version
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/
const char parse_arg_id[] = "@(#)$Id: $";

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include "parse_arg.h"
#include <string.h>


/*
*******************************************************************************
*                         LOCAL VARIABLES AND TABLES
*******************************************************************************
*/


/*
*******************************************************************************
*                         LOCAL PROGRAM CODE
*******************************************************************************
*/

int get_option(int argc, const char **argv, int arg_index, int num_opts, ...)
{
  va_list  ap;
  int      opt_index;
  char     *option;
  
  va_start(ap, num_opts);
  opt_index=0;
  while (opt_index<num_opts)
    {
      option = va_arg(ap, char*);
      if (strcmp(option, argv[arg_index])==0)
        break;
      opt_index++;
    }
  
  if (opt_index<num_opts)
    return opt_index;
  else
    return -1;
  
  va_end(ap);
}


const char* get_argument(int argc, const char **argv, int arg_index)
{
  if (arg_index > argc)
    return (char*)NULL;
  else
    return argv[(arg_index)];
}


/************************************************************************/


