/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : tonemod.c
*      Purpose          : modulator for the Cellular Text Telephone Modem
*                         1-out-of-4 tones (400, 600, 800, 1000 Hz)
*                         for the coding of each pair of two adjacent bits
*
*      Changes since October 13, 2000:
*      - The number samples that are generated by each call of tonemod()
*        can now be chosen without any constraints and must no longer be a
*        multiple of SYMB_LEN.
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/

#include "tonemod.h"
#include "ctm_defines.h"
#include "sin_fip.h"

#include <typedefs.h>
#include <stdlib.h>
#include <stdio.h>    

const char tonemod_id[] = "@(#)$Id: $" tonemod_h;

/*
******************************************************************************
*                         PUBLIC PROGRAM CODE
******************************************************************************
*/


/* ---------------------------------------------------------------------- */
/* Define an array for storing all possible waveforms. This array has to  */
/* be initialized using the function init_tonemod(), before the function  */
/* tonemod() can be used. The scope of this array is limited to this      */
/* module tonemod.c                                                       */
/* ---------------------------------------------------------------------- */

Shortint waveforms[SYMB_LEN][4];

/* ---------------------------------------------------------------------- */
/* Function init_tonemod()                                                */
/* ***********************                                                */
/* This function has to be executed before tonemod() can be used.         */
/* This function initializes the array waveforms[][]                      */
/* ---------------------------------------------------------------------- */

void init_tonemod(mod_state_t  *mod_state)
{
  Shortint cnt;

  for (cnt=0 ; cnt<SYMB_LEN; cnt++)
    {
      waveforms[cnt][0] = 
        8*sin_fip2047((Shortint)(((160/SYMB_LEN)*cnt*NCYCLES_0)%160));
      waveforms[cnt][1] = 
        8*sin_fip2047((Shortint)(((160/SYMB_LEN)*cnt*NCYCLES_1)%160));
      waveforms[cnt][2] = 
        8*sin_fip2047((Shortint)(((160/SYMB_LEN)*cnt*NCYCLES_2)%160));
      waveforms[cnt][3] = 
        8*sin_fip2047((Shortint)(((160/SYMB_LEN)*cnt*NCYCLES_3)%160));
    }
  /* Set Counter to its idle value */
  mod_state->cntModulatedSamples = SYMB_LEN;

  /* Set up fifo buffer for tx bits */
  Shortint_fifo_init(&(mod_state->txbits_fifo_state), 2*LENGTH_TX_BITS);
}


/* ---------------------------------------------------------------------- */
/* Function tonemod()                                                     */
/* ******************                                                     */
/* Modulator of the Cellular Text Telephone Modem.                        */
/*                                                                        */
/* New release (16. Nov. 2000):                                           */
/* The number samples that are generated by each call of this function    */
/* can now be chosen without any constraints and must no longer be a      */
/* multiple of SYMB_LEN.                                                  */
/* ---------------------------------------------------------------------- */

void tonemod(Shortint     *tones_out,
             Shortint     *bits_in,
             Shortint      num_samples_tones_out,
             Shortint      num_bits_in,
             mod_state_t  *mod_state)
{
  Shortint cntSamples;

  /* Push the input bits into the fifo buffer */
  if (num_bits_in>0)
    Shortint_fifo_push(&(mod_state->txbits_fifo_state), bits_in, num_bits_in);
  
  /* Generate as many samples as required */
  for (cntSamples=0; cntSamples<num_samples_tones_out; cntSamples++)
    {
      /* Check whether previous symbol was terminated */
      if (mod_state->cntModulatedSamples==SYMB_LEN)
        {
          /* Prepare the modulation of the next symbol. By default, the */
          /* next symbol consists of zero-valued samples...             */
          mod_state->actualBits[0]       = GUARD_BIT_SYMBOL;
          mod_state->actualBits[1]       = GUARD_BIT_SYMBOL;
          mod_state->cntModulatedSamples = 0;
          
          /* ... unless there are one or two bits for transmission */
          if (Shortint_fifo_check(&(mod_state->txbits_fifo_state))>=2)
            Shortint_fifo_pop(&(mod_state->txbits_fifo_state), 
                              mod_state->actualBits, 2);
          else if (Shortint_fifo_check(&(mod_state->txbits_fifo_state))==1)
            Shortint_fifo_pop(&(mod_state->txbits_fifo_state), 
                              mod_state->actualBits, 1);
        }

      /* Generate the Output Waveforms */
      if ((abs(mod_state->actualBits[0])==GUARD_BIT_SYMBOL) && 
          (abs(mod_state->actualBits[1])==GUARD_BIT_SYMBOL))
        {
          /* If both bits are carrying the guard symbol --> zero output */
          tones_out[cntSamples] = 0;
        }
      else
        {
          if (mod_state->actualBits[0]<=0)
            {
              if (mod_state->actualBits[1]<=0)
                tones_out[cntSamples] 
                  = waveforms[mod_state->cntModulatedSamples][0];
              else
                tones_out[cntSamples] 
                  = waveforms[mod_state->cntModulatedSamples][1];
            }
          else
            {
              if (mod_state->actualBits[1]<=0)
                tones_out[cntSamples] 
                  = waveforms[mod_state->cntModulatedSamples][2];
              else
                tones_out[cntSamples] 
                  = waveforms[mod_state->cntModulatedSamples][3];
            }
        }
      (mod_state->cntModulatedSamples)++;
    }
}
