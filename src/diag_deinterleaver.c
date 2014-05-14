/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : diag_deinterleaver.cc
*      Purpose          : diagonal (chain) deinterleaver routine
*
*      Changes since November 29, 2000:
*      - Bug in the last line of function shift_deinterleaver() corrected
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/

#include "diag_deinterleaver.h"
#include "init_interleaver.h"

#include <typedefs.h>

#include <stdio.h>    
#include <stdlib.h>

const char diag_deinterleaver_id[] = "@(#)$Id: $" diag_deinterleaver_h;


/*
*******************************************************************************
*                         PUBLIC PROGRAM CODE
*******************************************************************************
*/

void diag_deinterleaver(Shortint *out,
                        Shortint *in,
                        Shortint num_valid_bits,
                        interleaver_state_t *intl_state)
{
  Shortint cnt;
  Shortint i,j;
  
  for (cnt=0; cnt<num_valid_bits; cnt++)
    {   
      /* The input values are inserted into the last line of the */
      /* interleaver matrix and the output values are read out   */
      /* diagonally.                                             */
      intl_state->vector[(intl_state->B)*(intl_state->D)*(intl_state->B-1)+
                        (intl_state->clmn)] = in[cnt];
      
      out[cnt] = intl_state->vector[((intl_state->B)*(intl_state->D)+1)*
                                   (intl_state->clmn)]*(intl_state->scramble_vec[intl_state->clmn]);
      
      /* Increase the index of the actual column. If the end of the    */
      /* row is reached, the whole matrix is shifted by one row.       */
      intl_state->clmn++;
      
      if (intl_state->clmn == intl_state->B)
        {
          intl_state->clmn = 0;
          for (i=0; i<((intl_state->B)*(intl_state->D)-1); i++)
            for (j=0; j<(intl_state->B); j++)
              intl_state->vector[i*(intl_state->B)+j] = 
                intl_state->vector[(i+1)*(intl_state->B)+j];
        }
    }
}




void shift_deinterleaver(Shortint shift,
                         Shortint *insert_bits,
                         interleaver_state_t *ptr_state)
{
  Shortint cnt;
  
  if (shift>0) /* shift right */
    {
      for (cnt=(ptr_state->B-1)*(ptr_state->B)*(ptr_state->D)-1; 
           cnt-shift>=0; cnt--)
        ptr_state->vector[cnt] = ptr_state->vector[cnt-shift];
      
      for (cnt=0; cnt<shift; cnt++)
        ptr_state->vector[cnt] = insert_bits[cnt];
    }
  else
    {
      shift = abs(shift);
      for (cnt=0; cnt<(ptr_state->B-1)*(ptr_state->B)*(ptr_state->D)-shift;
           cnt++)
        ptr_state->vector[cnt] = ptr_state->vector[cnt+shift];
      
      for (cnt=0; cnt<shift; cnt++)
        ptr_state->vector[(ptr_state->B-1)*(ptr_state->B)*(ptr_state->D)-shift+cnt]
          = insert_bits[cnt];
    }
}



      
