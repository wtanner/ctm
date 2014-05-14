/*
*******************************************************************************
*
*      *******************************************************************************
*
*      File             : init_interleaver.c
*      Purpose          : initialization of the diagonal (chain) interleaver;
*                         definition of the type interleaver_state_t
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/
#include "typedefs.h"
#include "init_interleaver.h"
#include "m_sequence.h"

#include <stdio.h>    
#include <stdlib.h>

const char init_interleaver_id[] = "@(#)$Id: $" init_interleaver_h;


/*
*******************************************************************************
*                         PUBLIC PROGRAM CODE
*******************************************************************************
*/

void init_interleaver(interleaver_state_t *intl_state, 
                      Shortint B, Shortint D,
                      Shortint num_sync_lines1, Shortint num_sync_lines2)
{
  const Shortint demod_syncbits[] = { -1, -1, 1, -1, 1, 1, -1, 1 };
  
  Shortint cnt, num_dummy_bits, num_add_bits, num_avail_bits, seq_length;
  Shortint i,j,k;
      
  intl_state->B = B;
  intl_state->D = D;
  intl_state->scramble_vec = (Shortint*)calloc(B, sizeof(Shortint));
  intl_state->vector 
    = (Shortint*)calloc((num_sync_lines1+num_sync_lines2+B)*B*D, 
                        sizeof(Shortint));
  intl_state->rows = (num_sync_lines1+num_sync_lines2+B)*B*D/B;
  intl_state->clmn = 0;
  intl_state->ready = (num_sync_lines1 + num_sync_lines2);
  intl_state->num_sync_lines1 = num_sync_lines1;
  intl_state->num_sync_lines2 = num_sync_lines2;
  
  generate_scrambling_sequence(intl_state->scramble_vec, B);
  
  /* fill in the sync bits for the synchronization of the demodulator */
  
  for (cnt=0; cnt<num_sync_lines1*B; cnt++)
    intl_state->vector[cnt] = demod_syncbits[cnt%8];

  /* calculate, how many bits for the deinterleaver synchronization */
  /* are available                                                  */
  
  num_dummy_bits  = B*(B-1)*D/2;       /* dummy bits of the interleaver */
  num_add_bits    = num_sync_lines2*B; /* additional bits               */
  num_avail_bits  = num_dummy_bits+num_add_bits;
  
  intl_state->num_sync_bits = num_avail_bits;
  
  /* Determine the next value (2^n)-1 that is */
  /* greater or equal to num_avail_bits        */
  seq_length = 0;
  for (cnt=2; cnt<10; cnt++)
    if ((1<<cnt)-1 >=num_avail_bits)
      {
        seq_length = (1<<cnt)-1;
        break;
      }
  
  /* Calculate the m-sequence of the according length */

  intl_state->sequence = (Shortint*)calloc(seq_length,sizeof(Shortint));
  if (intl_state->sequence==(Shortint*)NULL)
    {
      fprintf(stderr,"Error while allocating memory for m-sequence\n");
      exit(1);
    }
  m_sequence(intl_state->sequence, seq_length);
  
  /* set-up a vector pointing to the bit positions that can be used for */
  /* storing the sync bits                                              */
  
  intl_state->sync_index_vec
    = (Shortint*)calloc(num_avail_bits, sizeof(Shortint));
  
  /* at first, the additional bits */

  for (cnt=0; cnt<num_add_bits; cnt++)
    intl_state->sync_index_vec[cnt] = cnt;
  
  /* now calculate the position of the interleaver's dummy bits */
  cnt = num_add_bits;
  
  for (i=0; i<B-1; i++)
    for (j=0; j<D; j++)
      for (k=i+1; k<B; k++)
        {
          intl_state->sync_index_vec[cnt] = num_add_bits + D*B*i + B*j + k;
          cnt++;
        }

  /* now fill all sync bits with the m_sequence */

  for (cnt=0; cnt<num_avail_bits; cnt++)
    intl_state->vector[num_sync_lines1*B + intl_state->sync_index_vec[cnt]] 
      = intl_state->sequence[cnt];

}





void reinit_interleaver(interleaver_state_t *intl_state)
{
  const Shortint demod_syncbits[] = { -1, -1, 1, -1, 1, 1, -1, 1 };
  
  Shortint cnt;
      
  intl_state->clmn = 0;

  /* fill in the sync bits for the synchronization of the demodulator */
  
  for (cnt=0; cnt<intl_state->num_sync_lines1*intl_state->B; cnt++)
    intl_state->vector[cnt] = demod_syncbits[cnt%8];

  /* now fill all sync bits with the m_sequence */

  for (cnt=0; cnt<intl_state->num_sync_bits; cnt++)
    intl_state->vector[intl_state->num_sync_lines1*intl_state->B + 
                      intl_state->sync_index_vec[cnt]] 
      = intl_state->sequence[cnt];
}




void init_deinterleaver(interleaver_state_t *intl_state, 
                       Shortint B, Shortint D)
{
  intl_state->B = B;
  intl_state->D = D;
  intl_state->scramble_vec = (Shortint*)calloc(B, sizeof(Shortint));
  intl_state->vector = (Shortint*)calloc(B*B*D, sizeof(Shortint));
  intl_state->clmn = 0;

  generate_scrambling_sequence(intl_state->scramble_vec, B);
}


void reinit_deinterleaver(interleaver_state_t *intl_state)
{
  intl_state->clmn = 0;
}


void calc_mute_positions(Shortint *mute_positions, 
                         Shortint num_rows_to_mute,
                         Shortint start_position,
                         Shortint B, 
                         Shortint D)
{
  Shortint row, clmn;
  Shortint cnt=0;
  
  for (row=0; row<num_rows_to_mute; row++)
    {
      for (clmn=0; clmn<B; clmn++)
        {
          mute_positions[cnt] = start_position + B*row + clmn*(B*D-1);
          // fprintf(stderr, ">>%d<< ", mute_positions[cnt]);
          cnt++;
        }
    }
}


Bool mutingRequired(Shortint  actualIndex, 
                    Shortint *mute_positions, 
                    Shortint  length_mute_positions)
{
  Shortint cnt;
  
  for (cnt=0; cnt<length_mute_positions; cnt++)
    if (actualIndex == mute_positions[cnt])
      return true;
  
  return false;
}


void generate_scrambling_sequence(Shortint *sequence, Shortint length)
{
  static const Shortint scramble_sequence[] 
    = {-1, 1, -1, -1, 1, 1, 
       -1,-1,-1, 1,1,1, -1,-1,-1,-1, 1,1,1,1,-1,-1,-1,-1,-1,1,1,1,1,1};
  Shortint cnt;
  
  if ((length > 30) || (length < 1))
    {
      fprintf(stderr, "Error in generate_scrambling_sequence():\n");
      fprintf(stderr, "No lengths > 30 supported yet!\n");
      exit(1);
    }
  
  for (cnt=0; cnt<length; cnt++)
    sequence[cnt] = scramble_sequence[cnt];
}
