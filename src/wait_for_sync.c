/*
*******************************************************************************
*
*
*******************************************************************************
*
*      File             : wait_for_sync.c
*      Purpose          : synchronization routine for the deinterleaver
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
#include "wait_for_sync.h"
#include "ctm_defines.h"
#include <stdio.h>    
#include <stdlib.h>            /* calloc() */

const char wait_for_sync_id[] = "@(#)$Id: $" wait_for_sync_h;


/*
*******************************************************************************
*                         PUBLIC PROGRAM CODE
*******************************************************************************
*/

void init_wait_for_sync(wait_for_sync_state_t *ptr_wait_state,
                        Shortint B, Shortint D,
                        Shortint num_sync_lines2)
{
  Shortint cnt;
  Shortint seq_length;
  Shortint maxindex;
  Shortint cntDiag;
  Shortint cntRow;
  Shortint cntResyncBits;
  Shortint index;
  Shortint num_dummy_bits;
  Shortint num_add_bits;
  Shortint i, j, k;
  Shortint *scrambling_sequence;

  /* Calculate the length of the preamble */
  
  num_dummy_bits  = B*(B-1)*D/2;       /* dummy bits of the interleaver */
  num_add_bits    = num_sync_lines2*B; /* additional bits               */

  ptr_wait_state->num_sync_bits = num_dummy_bits+num_add_bits;

  /* Allocate memory for a vector containing the positions of the initial */
  /* sync sequence (preamble). The preamble is located in the upper-right */
  /* triangular area of the interleaver matrix.                           */
  
  ptr_wait_state->sync_index_vec
    = (Shortint*)calloc(ptr_wait_state->num_sync_bits, sizeof(Shortint));
  
  /* Calculate the elements of sync_index_vec: first, the additional bits */
  for (cnt=0; cnt<num_add_bits; cnt++)
    ptr_wait_state->sync_index_vec[cnt] = cnt;
  
  /* Now calculate the positions of the interleaver's dummy bits */
  cnt = num_add_bits;
  for (i=0; i<B-1; i++)
    for (j=0; j<D; j++)
      for (k=i+1; k<B; k++)
        {
          ptr_wait_state->sync_index_vec[cnt] = num_add_bits + D*B*i + B*j + k;
          cnt++;
        }
  
  maxindex = ptr_wait_state->sync_index_vec[ptr_wait_state->num_sync_bits-1];
  
  /* Determine the next value (2^n)-1 that is          */
  /* greater or equal to ptr_wait_state->num_sync_bits */
  
  seq_length = 0;
  for (cnt=2; cnt<10; cnt++)
    if ((1<<cnt)-1 >= ptr_wait_state->num_sync_bits)
      {
        seq_length = (1<<cnt)-1;
        break;
      }

  /* Calculate the m-sequence of the according length */
  ptr_wait_state->m_sequence
    = (Shortint*)calloc(seq_length, sizeof(Shortint));
  if (ptr_wait_state->m_sequence==(Shortint*)NULL)
    {
      fprintf(stderr,"Error while allocating memory for m-sequence\n");
      exit(1);
    }
  m_sequence(ptr_wait_state->m_sequence, seq_length);

  /* Determine the next value (2^n)-1 that is */
  /* greater or equal to RESYNC_SEQ_LENGTH    */
  
  seq_length = 0;
  for (cnt=2; cnt<10; cnt++)
    if ((1<<cnt)-1 >= RESYNC_SEQ_LENGTH)
      {
        seq_length = (1<<cnt)-1;
        break;
      }
  
  /* Calculate the m-sequence of the according length */
  
  ptr_wait_state->m_sequence_resync 
    = (Shortint*)calloc(seq_length, sizeof(Shortint));
  if (ptr_wait_state->m_sequence_resync==(Shortint*)NULL)
    {
      fprintf(stderr,"Error while allocating memory for m-sequence\n");
      exit(1);
    }
  m_sequence(ptr_wait_state->m_sequence_resync, seq_length);
  
  /* The m-sequence has to be XOR-ed with the scrambling sequence */
  
  scrambling_sequence = (Shortint*)calloc(B, sizeof(Shortint));
  generate_scrambling_sequence(scrambling_sequence, B);
  
  for (cnt=0; cnt<seq_length; cnt++)
    ptr_wait_state->m_sequence_resync[cnt] 
      = ptr_wait_state->m_sequence_resync[cnt] * scrambling_sequence[cnt % B];
  free(scrambling_sequence);
  
  /* Allocate vector containing the positions where the elements of the  */
  /* resync sequence are located (the resync sequence is spread inside   */
    /* the bitstream coming from the demodulator due to the interleaving). */
  
  ptr_wait_state->resync_index_vec 
    = (Shortint*)calloc(RESYNC_SEQ_LENGTH, sizeof(Shortint));
  if (ptr_wait_state->resync_index_vec==(Shortint*)NULL)
    {
      fprintf(stderr,"Error while allocating memory for resync_index_vec\n");
      exit(1);
    }
  
  /* Calculate the positions of the resync sequence elements and  */
  /* determine the required length of the shift register.         */

  ptr_wait_state->length_shift_reg = 0;
  for (cntDiag=0; cntDiag < 1+(RESYNC_SEQ_LENGTH-1)/B; cntDiag++)
    for (cntRow=0; cntRow < B; cntRow++)
      {
        cntResyncBits = cntDiag*B + cntRow;
        if (cntResyncBits<RESYNC_SEQ_LENGTH)
          {
            index = cntDiag*B + cntRow*(D*B+1);
            
            ptr_wait_state->resync_index_vec[cntResyncBits] = index;
            
            if ((index+1) > ptr_wait_state->length_shift_reg)
              ptr_wait_state->length_shift_reg = index+1;
            
          }
      }

  /* Allocate memory for the shift registers */
  
  ptr_wait_state->shift_reg 
    = (Shortint*)calloc(ptr_wait_state->length_shift_reg, sizeof(Shortint));
  ptr_wait_state->xcorr1_shiftreg 
    = (Shortint*)calloc(ptr_wait_state->length_shift_reg, sizeof(Shortint));
  ptr_wait_state->xcorr2_shiftreg 
    = (Shortint*)calloc(ptr_wait_state->length_shift_reg, sizeof(Shortint));
  
  /* Since the resync sequence is spread over a longer time interval than */
  /* the preamble, for the detection of the preamble the shift register   */
  /* is longer than required. Since the register contains the most actual */
  /* bits at the positions with the highest indices, we have to add an    */
  /* offset to all indices in sync_index_vec in order to achieve a low    */
  /* delay of the preamble detection.                                     */
  
  ptr_wait_state->offset = ptr_wait_state->length_shift_reg - maxindex -1;
  for (cnt=0; cnt<ptr_wait_state->num_sync_bits; cnt++)
    ptr_wait_state->sync_index_vec[cnt] += ptr_wait_state->offset;
  
  ptr_wait_state->sync_found         = false;
  ptr_wait_state->alreadyCTMreceived = false;
  ptr_wait_state->cntSymbolsSinceEndOfBurst = maxUShortint;
}


/* *************************************************************************/


void reinit_wait_for_sync(wait_for_sync_state_t *ptr_wait_state)
{
  ptr_wait_state->sync_found = false;
  ptr_wait_state->cntSymbolsSinceEndOfBurst = 0;
}


/* *************************************************************************/


Bool wait_for_sync(Shortint *out_bits,
                   Shortint *in_bits,
                   Shortint  num_in_bits,
                   Shortint  num_received_idle_symbols,
                   Shortint  *ptr_num_valid_out_bits,
                   Shortint  *ptr_wait_interval,
                   Shortint  *ptr_resync_detected,
                   Bool      *ptr_early_muting_required,
                   wait_for_sync_state_t *ptr_wait_state)
{
  Shortint  cnt, sampl_cnt;
  Shortint  xcorr = 0;
  Shortint  xcorr_resync = 0;
  Bool      actual_sync_found = false;
  Bool      sampleIsTone;
  Shortint  actual_threshold;
  Shortint  actual_sample;
  Shortint  index;
  Shortint  max_xcorr1;
  Shortint  max_xcorr2;
    
#ifdef DEBUG_OUTPUT
  double       dbl_value;
  static Bool  firsttime=true;
  static FILE  *sync_xcorr_file;
  static FILE  *resync_xcorr_file;
  
  if (firsttime)
    {
      firsttime=false;
      if ((sync_xcorr_file=fopen("sync_xcorr_info.dbl", "wb"))==NULL)
        {
          fprintf(stderr,"Error while opening sync_xcorr_info.dbl\n\n") ;
          exit(1);
        }
      if ((resync_xcorr_file=fopen("resync_xcorr_info.dbl", "wb"))==NULL)
        {
          fprintf(stderr,"Error while opening resync_xcorr_info.dbl\n\n") ;
          exit(1);
        }
    }
#endif
  
  *ptr_num_valid_out_bits = 0;
  *ptr_resync_detected = -1;


  /*************************************************************************/
  /* Now we have to calculate the cross-correlation functions between the  */
  /* received bit-stream and copies of the preamble and the                */
  /* resynchronization sequence, respectively. Due to the interleaving the */
  /* preamble and the resynchronization sequence don't appear coherently   */
  /* in the received bit-stream. The straightforward implementation for    */
  /* calculating the cross-correlation would be an double-indexed          */
  /* addressing, i.e. the incoming bit-stream is buffered in a shift       */
  /* and the elements that contribute to the cross-correlation are         */
  /* via a look-up table with the correct indices:                         */
  /*                                                     +--------------+  */
  /*     +---*-----*-----------*---------------------*---| index table  |  */
  /*     |   |     |           |                     |   +--------------+  */
  /*     |   |     |           |                     |                     */
  /*     v   v     v           v                     v                     */
  /* +----------------------------------------------------+  RX bitstream  */
  /* |                  shift register                    |<-------------  */
  /* +----------------------------------------------------+                */
  /*     |   |     |           |                     |                     */
  /*     |   |     |           |                     |                     */
  /*     v   v     v           v                     v                     */
  /*   +-----------------------------------------------+                   */
  /*   |                  correlate                    |------>  output    */
  /*   +-----------------------------------------------+                   */
  /*                                                                       */
  /* In order to implement an early detection of both sequences (this is   */
  /* is required for an early blocking of the audio signal), we use an     */
  /* equivalent implementation, where the correlation functions rather     */
  /* than the received bit-stream is stored in a shift register:           */
  /*                                                                       */
  /*                                                         RX bitstream  */
  /*                                                       +-------------  */
  /*                                                       |               */
  /*       +----------------------*---------------*----*---*               */
  /*       |                      |               |    |   |               */
  /*       v                      v               v    v   v               */
  /*    +----------------------------------------------------+   0,0,...0  */
  /* +--|               shift register + correlate           |<----------  */
  /* |  +----------------------------------------------------+             */
  /* |                                                                     */
  /* |                                                                     */
  /* v output                                                              */
  /*                                                                       */
  /* The correlation itself is made by means of a modified correlation     */
  /* operation, which considers also bits that have been marked as         */
  /* unreliable by the receiver. For each received unreliable bit, the     */
  /* resulting correlation value is reduced by a value of 0.5.             */
  /*                                                                       */
  /*************************************************************************/

     
  for (sampl_cnt=0; sampl_cnt<num_in_bits; sampl_cnt++)
    {
      /* Update of the shift register: all elements are shifted towards    */
      /* lower indices and new elements are inserted at the highest index. */
      /* This shift register is NOT required for calculating the           */
      /* correlation values, but it's neccessary for restoring the output  */
      /* bit-stream after the synchronization has been detected.           */
      
      for (cnt=0; cnt<ptr_wait_state->length_shift_reg-1; cnt++)
        ptr_wait_state->shift_reg[cnt] = ptr_wait_state->shift_reg[cnt+1];
      ptr_wait_state->shift_reg[ptr_wait_state->length_shift_reg-1] 
        = in_bits[sampl_cnt];
      
      /* Correlation between the received bitstream and the preamble */

      for (cnt=0; cnt<ptr_wait_state->length_shift_reg-1; cnt++)
        ptr_wait_state->xcorr1_shiftreg[cnt] 
          = ptr_wait_state->xcorr1_shiftreg[cnt+1];
      ptr_wait_state->xcorr1_shiftreg[ptr_wait_state->length_shift_reg-1] = 0;
      
      for (cnt=0; cnt<ptr_wait_state->num_sync_bits; cnt++)
        {
          actual_sample = ptr_wait_state->m_sequence[cnt] * in_bits[sampl_cnt];
          sampleIsTone  = (((actual_sample & 0x0001)!=0) || 
                           (ptr_wait_state->cntSymbolsSinceEndOfBurst<600));
          index = (ptr_wait_state->length_shift_reg-1
                   -ptr_wait_state->sync_index_vec[cnt]);
          if (sampleIsTone && 
              (abs(actual_sample) > THRESHOLD_RELIABILITY_FOR_XCORR))
            {
              if (actual_sample >0)
                ptr_wait_state->xcorr1_shiftreg[index] += 2;
              else
                ptr_wait_state->xcorr1_shiftreg[index] -= 2;
            }
          else
            ptr_wait_state->xcorr1_shiftreg[index]--;
        }
      xcorr = ptr_wait_state->xcorr1_shiftreg[0]>>1;
      
      /* Correlation between the received bitstream and the resync sequence. */

      for (cnt=0; cnt<ptr_wait_state->length_shift_reg-1; cnt++)
        ptr_wait_state->xcorr2_shiftreg[cnt] 
          = ptr_wait_state->xcorr2_shiftreg[cnt+1];
      ptr_wait_state->xcorr2_shiftreg[ptr_wait_state->length_shift_reg-1] = 0;
      
      for (cnt=0; cnt<RESYNC_SEQ_LENGTH; cnt++)
        {
          actual_sample 
            = ptr_wait_state->m_sequence_resync[cnt] * in_bits[sampl_cnt];
          sampleIsTone  = (((actual_sample & 0x0001)!=0) || 
                           (ptr_wait_state->cntSymbolsSinceEndOfBurst<600));
          index = (ptr_wait_state->length_shift_reg-1
                   -ptr_wait_state->resync_index_vec[cnt]);
          if (sampleIsTone)
            {
              if (actual_sample >0)
                ptr_wait_state->xcorr2_shiftreg[index] += 2;
              else
                ptr_wait_state->xcorr2_shiftreg[index] -= 2;
            }
          else
            ptr_wait_state->xcorr2_shiftreg[index]--;
        }
      xcorr_resync = ptr_wait_state->xcorr2_shiftreg[0]>>1;
      

      /* Define the threshold for detecting the synchronization sequence.  */
      /* If already InSync, threshold2 is used which should be greater     */
      /* than threshold1, which is used when the receiver is  waiting      */
      /* for the next synchronization burst. If no CTM burst has been      */
      /* received so far, threshold0 is be used, which should be greater   */
      /* threshold1 in order to avoid false-detection in pure voice calls. */
      
      if ((ptr_wait_state->sync_found) && 
          (num_received_idle_symbols<MAX_IDLE_SYMB-1))
        actual_threshold = WAIT_SYNC_REL_THRESHOLD_2;
      else if (ptr_wait_state->alreadyCTMreceived)
        actual_threshold = WAIT_SYNC_REL_THRESHOLD_1;
      else 
        actual_threshold = WAIT_SYNC_REL_THRESHOLD_0;

      /* Decide whether the "early muting" of the output signal is        */
      /* neccesary. The "early muting" is a function that blocks the      */
      /* bypassing path of the audio signal even before the               */
      /* synchronization is detected. This is to guarantee that the       */
      /* preamble or resync sequence is detected only by the first CTM    */
      /* device, if several CTM devices are cascaded subsequently.        */
      
      *ptr_early_muting_required = false;
      max_xcorr1=0;
      max_xcorr2=0;
      for (cnt=0; cnt<ptr_wait_state->length_shift_reg; cnt++)
        {
          if (ptr_wait_state->xcorr1_shiftreg[cnt] > max_xcorr1)
            max_xcorr1 = ptr_wait_state->xcorr1_shiftreg[cnt];
          if (ptr_wait_state->xcorr2_shiftreg[cnt] > max_xcorr2)
            max_xcorr2 = ptr_wait_state->xcorr2_shiftreg[cnt];
        }
      max_xcorr1 = max_xcorr1>>1;
      max_xcorr2 = max_xcorr2>>1;
      if ((((Longint)(max_xcorr2)<<15) > 
           (Longint)RESYNC_REL_THRESHOLD*RESYNC_SEQ_LENGTH) ||
          ((((Longint)(max_xcorr1)<<15) > 
            (Longint)actual_threshold*ptr_wait_state->num_sync_bits)))      
        *ptr_early_muting_required = true;
      
      
      /* Detection of the resync sequence */
      
      if (((Longint)(xcorr_resync)<<15) > 
          (Longint)RESYNC_REL_THRESHOLD*RESYNC_SEQ_LENGTH)
        {
          *ptr_resync_detected = *ptr_num_valid_out_bits;
        }
      
      if ((*ptr_resync_detected >=0) && !(ptr_wait_state->sync_found))
        {
          /* If the resync sequence is detected and the receiver is not   */
          /* "in sync" at the moment, this is used as an initial          */
          /* synchronization, i.e. the receiver is set into the "in sync" */
          /* state and the shift register's contents is copied to the     */
          /* output.                                                      */
          
          actual_sync_found  = true;
          ptr_wait_state->alreadyCTMreceived        = true;
          ptr_wait_state->sync_found                = true;
          ptr_wait_state->cntSymbolsSinceEndOfBurst = 0;
          *ptr_wait_interval = RESYNC_SEQ_LENGTH;
          
          for (cnt=0; cnt<ptr_wait_state->length_shift_reg; cnt++)
            out_bits[cnt] = ptr_wait_state->shift_reg[cnt];
          
          *ptr_num_valid_out_bits = ptr_wait_state->length_shift_reg;
        }
      /* If the resync sequence has not been detected: try to detect        */
      /* the initial synchronization sequence (preamble).                   */
      /* This detector is active even if the receiver is already "in sync". */
      else if (((Longint)(xcorr)<<15) > 
               (Longint)actual_threshold*ptr_wait_state->num_sync_bits)
        {
          actual_sync_found  = true;
          ptr_wait_state->alreadyCTMreceived        = true;
          ptr_wait_state->sync_found                = true;
          ptr_wait_state->cntSymbolsSinceEndOfBurst = 0;
          *ptr_wait_interval = 0;
          
          /* If the initial sync is detected, the shift register's       */
          /* contents is copied to the output so that wait_for_sync()    */
          /* is transparant and causes no delay in the future.           */
          
          for (cnt=0; cnt<ptr_wait_state->length_shift_reg-ptr_wait_state->offset; cnt++)
            out_bits[cnt] 
              = ptr_wait_state->shift_reg[cnt+ptr_wait_state->offset];
          
          *ptr_num_valid_out_bits 
            = ptr_wait_state->length_shift_reg - ptr_wait_state->offset;
        }
      /* If there is actually no synchronization detected, but if the */
      /* synchronization has already been detected earlier, the       */
      /* incoming bits are copied to the output                       */
      else if (ptr_wait_state->sync_found)
        {
          out_bits[*ptr_num_valid_out_bits] = in_bits[sampl_cnt];
          *ptr_num_valid_out_bits = *ptr_num_valid_out_bits+1;
        }
      /* If no synchronization has been detected (neither during this */
      /* frame nor during earlier frames), increase the counter for   */
      /* the frames since the termination of the last CTM burst.      */
      else
        {
          if (ptr_wait_state->cntSymbolsSinceEndOfBurst<maxUShortint)
            ptr_wait_state->cntSymbolsSinceEndOfBurst++;
        }
      
      
#ifdef DEBUG_OUTPUT
      dbl_value = (double)xcorr;
      if (fwrite(&dbl_value, sizeof(double), 1, sync_xcorr_file) == 0)
        {
          fprintf(stderr,"Error while writing to sync_xcorr_info.dbl\n\n");
          exit(1);
        }
      dbl_value = (double)xcorr_resync;
      dbl_value = (double)(ptr_wait_state->xcorr1_shiftreg[0]);
      
      if (fwrite(&dbl_value, sizeof(double), 1, resync_xcorr_file) == 0)
        {
          fprintf(stderr,"Error while writing to resync_xcorr_info.dbl\n\n");
          exit(1);
        }
#endif
    }
  return actual_sync_found;
}




void generate_resync_sequence(Shortint *sequence)
{
  Shortint seq_length_tmp;
  Shortint cnt;
  
  Shortint *sequence_tmp;
  
  /* Determine the next value (2^n)-1 that is */
  /* greater or equal to seq_length           */
  seq_length_tmp = 0;
  for (cnt=2; cnt<10; cnt++)
    if ((1<<cnt)-1 >= RESYNC_SEQ_LENGTH)
      {
        seq_length_tmp = (1<<cnt)-1;
        break;
      }
  
  /* allocate + calculate the m-sequence of length (2^n)-1 */
  sequence_tmp = (Shortint*)calloc(seq_length_tmp,sizeof(Shortint));
  if (sequence_tmp==(Shortint*)NULL)
    {
      fprintf(stderr,"Error while allocating memory for m-sequence\n");
      exit(1);
    }
  m_sequence(sequence_tmp, seq_length_tmp);
  
  /* copy the first seq_length samples into output vector */
  for (cnt=0; cnt<RESYNC_SEQ_LENGTH; cnt++)
    sequence[cnt] = sequence_tmp[cnt];
  
  free(sequence_tmp);
}
