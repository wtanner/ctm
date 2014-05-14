/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : diag_interleaver.c
*      Purpose          : diagonal (chain) interleaver routine
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/
#include "diag_interleaver.h"
#include "init_interleaver.h"
#include "ctm_defines.h"

#include <typedefs.h>

#include <stdio.h>    
#include <stdlib.h>

const char diag_interleaver_id[] = "@(#)$Id: $" diag_interleaver_h;


/*
*******************************************************************************
*                         PUBLIC PROGRAM CODE
*******************************************************************************
*/

void diag_interleaver(Shortint *out,
                      Shortint *in,
                      Shortint  num_bits,
                      interleaver_state_t *intl_state)
{
  Shortint cnt, syncoffset;
  Shortint i,j;
  
  for (cnt=0; cnt<num_bits; cnt++)
    { 
      /* The input values are diagonally inserted into the interleaver */
      /* matrix and the output values are read out line by line.       */
      
      syncoffset = (intl_state->B)*
        ((intl_state->num_sync_lines1)+(intl_state->num_sync_lines2));
      
      intl_state->vector[syncoffset + 
                        ((intl_state->B)*(intl_state->D)+1)*(intl_state->clmn)]
        = in[cnt]*(intl_state->scramble_vec[intl_state->clmn]);
      
      out[cnt] = intl_state->vector[intl_state->clmn];
      
      /* Increase the index of the actual column. If the end of the    */
      /* row is reached, the whole matrix is shifted by one row.       */
      intl_state->clmn++;
      if (intl_state->clmn == intl_state->B)
        {
          intl_state->clmn = 0;
          for (i=0; i<((intl_state->B)*(intl_state->D)-1+
                       (intl_state->num_sync_lines1)+
                       (intl_state->num_sync_lines2)); i++)
            for (j=0; j<(intl_state->B); j++)
              intl_state->vector[i*(intl_state->B)+j] = 
                intl_state->vector[(i+1)*(intl_state->B)+j];
        }
    }
}


void diag_interleaver_flush(Shortint *out,
                            Shortint  *num_bits_out,
                            interleaver_state_t *intl_state)
{
  Shortint cnt, syncoffset;
  Shortint i,j;
  
  *num_bits_out = (((intl_state->B-1))*(intl_state->D) +
                   (intl_state->num_sync_lines1)+
                   (intl_state->num_sync_lines2)) * intl_state->B;
  
  for (cnt=0; cnt < *num_bits_out; cnt++) 
    { 
      /* The input values are diagonally inserted into the interleaver */
      /* matrix and the output values are read out line by line.       */
      
      syncoffset = (intl_state->B)*
        ((intl_state->num_sync_lines1)+(intl_state->num_sync_lines2));
      
      intl_state->vector[syncoffset + 
                        ((intl_state->B)*(intl_state->D)+1)*(intl_state->clmn)]
        = (intl_state->scramble_vec[intl_state->clmn]);

      out[cnt] = intl_state->vector[intl_state->clmn];
          
      /* Increase the index of the actual column. If the end of the    */
      /* row is reached, the whole matrix is shifted by one row.       */
      intl_state->clmn = (intl_state->clmn) + 1;
      if (intl_state->clmn == intl_state->B)
        {
          intl_state->clmn = 0;
          for (i=0; i<((intl_state->B)*(intl_state->D)-1+
                       (intl_state->num_sync_lines1)+
                       (intl_state->num_sync_lines2)); i++)
            for (j=0; j<(intl_state->B); j++)
              intl_state->vector[i*(intl_state->B)+j] = 
                intl_state->vector[(i+1)*(intl_state->B)+j];
        }
    }
}

