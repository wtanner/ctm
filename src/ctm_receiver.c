/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : ctm_receiver.c
*      Author           : EEDN/RV Matthias Doerbecker
*      Tested Platforms : Sun Solaris, MS Windows NT 4.0
*      Description      : Complete CTM Receiver including Demodulator, 
*                         Synchronisation, Deinterleaver, and Error Correction
*
*      Changes since October 13, 2000:
*      - added reset function reset_ctm_receiver()
*
*      Changes since December 07, 2000:
*      - Bug fix within the code for initial synchronization 
*        based on the detection of the resync sequence
*
*      $Log: $
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/
#include "ctm_receiver.h"
const char ctm_receiver_id[] = "@(#)$Id: $" ctm_receiver_h;

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include "ctm_defines.h"
#include "init_interleaver.h"
#include "diag_deinterleaver.h"
#include "tonedemod.h"
#include "m_sequence.h"
#include "wait_for_sync.h"
#include "conv_poly.h"
#include "viterbi.h"
#include "ucs_functions.h"

#include <typedefs.h>
#include <fifo.h>

#include <stdlib.h>              /* calloc() */
#include <stdio.h> 

/***********************************************************************/
/* init_ctm_receiver()                                                 */
/* *******************                                                 */
/* Initialization of the CTM Receiver.                                 */
/*                                                                     */
/* output vaiables:                                                    */
/* rx_state :   pointer to a variable of rx_state_t containing the     */
/*              initialized states of the receiver                     */
/***********************************************************************/

void init_ctm_receiver(rx_state_t* rx_state)
{
  rx_state->samplingCorrection        = 0;
  rx_state->cntIdleSymbols            = 0;
  rx_state->numDeintlBits             = 0;
  rx_state->cntRXBits                 = 0;
  rx_state->syncCorrect               = 0;
  rx_state->cntUnreliableGrossBits    = 0;
  
  /* set up fifo buffers */
  Shortint_fifo_init(&(rx_state->rx_bits_fifo_state), 
                     intlvB*intlvB*intlvD+CHC_RATE);
  Shortint_fifo_init(&(rx_state->net_bits_fifo_state), 
                     (intlvB*intlvB*intlvD)/CHC_RATE+3+BITS_PER_SYMB);
  
  Shortint_fifo_init(&(rx_state->octet_fifo_state), 5);
  
  /* Initialize the demodulator */
  init_tonedemod(&(rx_state->tonedemod_state));

  /* Initialize the viterbi decoder */
  viterbi_init(&(rx_state->viterbi_state));
  
  calc_mute_positions(rx_state->mutePositions, NUM_MUTE_ROWS, intlvB-1,
                      intlvB, intlvD);
  
  /* initialize interleaver/deinterleaver/wait_for_sync */
  /* and allocate memory for input/output vectors       */
  
  init_deinterleaver(&(rx_state->deintl_state), intlvB, intlvD);

  init_wait_for_sync(&(rx_state->wait_state), intlvB, intlvD, deintSyncLns);
  
  rx_state->waitSyncOut 
    = (Shortint*)calloc(2+rx_state->wait_state.length_shift_reg-1, sizeof(Shortint));
  if (rx_state->waitSyncOut==(Shortint*)NULL)
    {
      fprintf(stderr,"Error while allocating memory for waitSyncOut\n");
      exit(1);
    }
  
  rx_state->deintlOut 
    = (Shortint*)calloc(2+rx_state->wait_state.length_shift_reg-1, sizeof(Shortint));
  if (rx_state->deintlOut==(Shortint*)NULL)
    {
      fprintf(stderr,"Error while allocating memory for deintlOut\n");
      exit(1);
    }
}

void reset_ctm_receiver(rx_state_t* rx_state)
{
  rx_state->samplingCorrection        = 0;
  rx_state->cntIdleSymbols            = 0;
  rx_state->numDeintlBits             = 0;
  rx_state->cntRXBits                 = 0;
  rx_state->syncCorrect               = 0;
  rx_state->cntUnreliableGrossBits    = 0;
  
  /* reset fifo buffers */
  Shortint_fifo_reset(&(rx_state->rx_bits_fifo_state));
  Shortint_fifo_reset(&(rx_state->net_bits_fifo_state));
  Shortint_fifo_reset(&(rx_state->octet_fifo_state));
  
  /* reset the viterbi decoder */
  viterbi_reinit(&(rx_state->viterbi_state));
  
  reinit_deinterleaver(&(rx_state->deintl_state));
  reinit_wait_for_sync(&(rx_state->wait_state));
}


/***************************************************************************/
/* ctm_receiver()                                                          */
/* **************                                                          */
/* Runs the Cellular Text Telephone Modem Receiver for a block of          */
/* (nominally) 160 samples. Due to the internal synchronization, the       */
/* number of processed samples might vary between 156 and 164 samples.     */
/* The input of the samples and the output of the decoded characters       */
/* is handled via fifo buffers, which have to be initialized               */
/* externally before using this function (see fifo.h for details).         */
/*                                                                         */
/* input/output variables:                                                 */
/* *ptr_signal_fifo_state      fifo state for the input samples            */
/* *ptr_output_char_fifo_state fifo state for the output characters        */
/* *ptr_early_muting_required  returns whether the original audio signal   */
/*                             must not be forwarded. This is to guarantee */
/*                             that the preamble or resync sequence is     */
/*                             detected only by the first CTM device, if   */
/*                             several CTM devices are cascaded            */
/*                             subsequently.                               */
/* *rx_state                   pointer to the variable containing the      */
/*                             receiver states                             */
/***************************************************************************/

void ctm_receiver(fifo_state_t*  ptr_signal_fifo_state,
                  fifo_state_t*  ptr_output_char_fifo_state,
                  Bool*          ptr_early_muting_required,
                  rx_state_t*    rx_state)
{
  Shortint  toneVec[SYMB_LEN+1];
  Shortint  bitsDemod[2];
  Shortint  cnt;
  Shortint  numValidBits;
  Bool      actual_sync_found;
  Bool      octetAvailable;
  Shortint  fecNetBit;
  UShortint utfOctet = 0;
  Shortint  ShortintValueTmp;
  Shortint  syncOffset;
  Shortint  resyncDetected;
  Shortint  wait_interval;
  Shortint  numViterbiOutBits;
  UShortint ucsCode  = 0;

  static Shortint  ucsBits[BITS_PER_SYMB];
  static Shortint  fecGrossBitsIn[CHC_RATE];

#ifdef DEBUG_OUTPUT
  static Bool      firsttime = true;
  static FILE      *rx_bits_file;
  if (firsttime)
    {
      firsttime = false;
      if ((rx_bits_file=fopen("rx_bits.srt", "wb"))==NULL)
        {
          fprintf(stderr, "Error while opening rx_bits.srt\n\n") ;
          exit(1);
        }
    } 
#endif  
  
  while (Shortint_fifo_check(ptr_signal_fifo_state)>SYMB_LEN)
    {
      /* Pop SYMB_LEN-1, SYMB_LEN, or SYMB_LEN+1 samples from fifo, */
      /* depending on the state of samplingCorrection.              */
      Shortint_fifo_pop(ptr_signal_fifo_state, toneVec, 
                        SYMB_LEN+rx_state->samplingCorrection);
      
      /* Run the tone demodulator */
      tonedemod(bitsDemod, toneVec, 
                (Shortint)(SYMB_LEN+rx_state->samplingCorrection), 
                &(rx_state->samplingCorrection), 
                &(rx_state->tonedemod_state));
      
#ifdef DEBUG_OUTPUT
      if (fwrite(bitsDemod, sizeof(Shortint), 2, rx_bits_file) == 0)
        {
          fprintf(stderr, "Error while writing to file\n\n") ;
          exit(1);
        }
#endif
      
      /* Find the synchronization sequence and run the */
      /* deinterleaver on the synchronized bitstream   */
      actual_sync_found = 
        wait_for_sync(rx_state->waitSyncOut, bitsDemod, 2, 
                      rx_state->cntIdleSymbols, &numValidBits, 
                      &wait_interval, &resyncDetected, 
                      ptr_early_muting_required,
                      &(rx_state->wait_state));
      
      if (actual_sync_found)
        {
#ifdef DEBUG_OUTPUT
          if (wait_interval==0)
            fprintf(stderr, ">>>Initial sync found<<<");
          else 
            fprintf(stderr, ">>>Initial sync found (based on resync sequence)<<<");
#endif
          resyncDetected = -1;
          reinit_deinterleaver(&(rx_state->deintl_state));
          viterbi_reinit(&(rx_state->viterbi_state));
          rx_state->cntIdleSymbols = 0;
          rx_state->numDeintlBits = 0;
          rx_state->cntRXBits = 0;
          rx_state->syncCorrect = 0;
          
          /* Pop all remaining bits from fifos and forget the bits */
          Shortint_fifo_reset(&(rx_state->net_bits_fifo_state));
          Shortint_fifo_reset(&(rx_state->rx_bits_fifo_state));

          /* Calculate the deinterleaver's delay of                      */
          /* intlvB*deintSyncLns+intlvB*(intlvB-1)*intlvD elements       */
          /* plus an additional delay of RESYMC_SEQ_LENGTH, if the       */
          /* synchronization was triggered by detecting resync sequence  */
          /* and not by detecting the preamble.                          */
          rx_state->intl_delay 
            = intlvB*deintSyncLns+intlvB*(intlvB-1)*intlvD + wait_interval;
        }

      /**************************************************************/
      /*       The following lines are for resynchronisation        */
      /*         (also the deinterleaver is executed here)          */
      /**************************************************************/
      
      if (resyncDetected>=0)
        {
          syncOffset 
            = (rx_state->numDeintlBits + resyncDetected + 1
               - rx_state->intl_delay)
            % (NUM_BITS_BETWEEN_RESYNC+RESYNC_SEQ_LENGTH);
          
          if (syncOffset
              > (NUM_BITS_BETWEEN_RESYNC+RESYNC_SEQ_LENGTH)/2)
            syncOffset = syncOffset 
              - (NUM_BITS_BETWEEN_RESYNC+RESYNC_SEQ_LENGTH);
          
          if ((syncOffset>-16) && (syncOffset<16))
            rx_state->syncCorrect = syncOffset;
          else 
            rx_state->syncCorrect = 0;
          
#ifdef DEBUG_OUTPUT
          if (rx_state->syncCorrect!=0)
            fprintf(stderr, ">>resync detected: %d<<", syncOffset);
#endif
        }
      
      if (rx_state->syncCorrect==0)
        {
          /* The synchronisation is still ok. --> no adaptation */
          diag_deinterleaver(rx_state->deintlOut, rx_state->waitSyncOut,
                             numValidBits, &(rx_state->deintl_state));
        }
      else if (rx_state->syncCorrect>0)
        {
          /* The ResyncSecquence was too late */
          /* --> some bits have to be dropped */
          if (rx_state->syncCorrect>=numValidBits)
            {
              /* The incoming bits are only inserted into the deinterleaver, */
              /* no bits are handled to the following functions              */
              shift_deinterleaver((Shortint)(-numValidBits), 
                                  rx_state->waitSyncOut,
                                  &(rx_state->deintl_state));
              rx_state->syncCorrect -= numValidBits;
              numValidBits=0;
            }
          else
            {
              shift_deinterleaver((Shortint)(-rx_state->syncCorrect),
                                  rx_state->waitSyncOut,
                                  &(rx_state->deintl_state));
              
              numValidBits -= rx_state->syncCorrect;
              diag_deinterleaver(rx_state->deintlOut, 
                                 &(rx_state->waitSyncOut[rx_state->syncCorrect]),
                                 numValidBits, &(rx_state->deintl_state));
              rx_state->syncCorrect=0;
            }
        }
      else
        {
          /* The ResyncSequence was too early        */
          /* --> additional Bits have to be inserted */
          ShortintValueTmp=0;
          for (cnt=0; cnt<-(rx_state->syncCorrect); cnt++)
            {
              diag_deinterleaver(&(rx_state->deintlOut[cnt]), 
                                 &ShortintValueTmp,
                                 1, &(rx_state->deintl_state));
              shift_deinterleaver(1, &ShortintValueTmp,
                                  &(rx_state->deintl_state));
            }
          diag_deinterleaver(&(rx_state->deintlOut[-(rx_state->syncCorrect)]), 
                             rx_state->waitSyncOut,
                             numValidBits, &(rx_state->deintl_state));
          numValidBits += -(rx_state->syncCorrect);
          rx_state->syncCorrect=0;
        }
      
      /**************************************************************/
      /*               End of resynchronisation                     */
      /**************************************************************/
      
      /* Consider the deinterleaver's delay                         */
      /* and push the demodulated bits into the fifo buffer         */
      for (cnt=0; cnt<numValidBits; cnt++)
        {
          if (rx_state->numDeintlBits >= rx_state->intl_delay)
            {
              /* Ignore all bits that are for muting or resync */
              if (!(mutingRequired((Shortint)(rx_state->cntRXBits), 
                                   rx_state->mutePositions, 
                                   NUM_MUTE_ROWS*intlvB))
                  && (rx_state->cntRXBits<NUM_BITS_BETWEEN_RESYNC))
                {
                  Shortint_fifo_push(&(rx_state->rx_bits_fifo_state), 
                                     &(rx_state->deintlOut[cnt]), 1);
                }
              
              rx_state->cntRXBits++;
              if(rx_state->cntRXBits
                 ==NUM_BITS_BETWEEN_RESYNC+RESYNC_SEQ_LENGTH)
                rx_state->cntRXBits = 0;
            }
          rx_state->numDeintlBits++;

          /* Avoid Overflows of numDeintlBits */
          if (rx_state->numDeintlBits > 10000)
            rx_state->numDeintlBits 
              -= (NUM_BITS_BETWEEN_RESYNC+RESYNC_SEQ_LENGTH);
        } 
    }
  
  /* As long as there are gross bits in the fifo: pop them, run  */
  /* the channel decoder and pop the net bits into the next fifo */
  while (Shortint_fifo_check(&(rx_state->rx_bits_fifo_state)) > CHC_RATE)
    {
      Shortint_fifo_pop(&(rx_state->rx_bits_fifo_state), 
                        fecGrossBitsIn, CHC_RATE);
      
      /* Count gross bits with low reliability (i.e. bits with too low */
      /* magnitute or with their LSB not set).                         */
      for (cnt=0;cnt<CHC_RATE; cnt++)
        if ((abs(fecGrossBitsIn[cnt])<THRESHOLD_RELIABILITY_FOR_GOING_OFFLINE)
            || ((fecGrossBitsIn[cnt] & 0x0001) == 0))
          {
            if (rx_state->cntUnreliableGrossBits < maxShortint)
              rx_state->cntUnreliableGrossBits++;
          }
        else
          rx_state->cntUnreliableGrossBits=0;

      /* Channel decoder */
      viterbi_exec(fecGrossBitsIn, 1*CHC_RATE, 
                   &fecNetBit, &numViterbiOutBits,
                   &(rx_state->viterbi_state));
      if (numViterbiOutBits > 0)
        {
          Shortint_fifo_push(&(rx_state->net_bits_fifo_state), &fecNetBit, 1);
        }
    }
  
  octetAvailable = false;
  
  /* As long as there are bits on the fifo: pop them and decode them into */
  /* octets until a valid octet (i.e. not an idle symbol) is received     */
  while (Shortint_fifo_check(&(rx_state->net_bits_fifo_state))>=BITS_PER_SYMB)
    {
      Shortint_fifo_pop(&(rx_state->net_bits_fifo_state),
                        ucsBits, BITS_PER_SYMB);
      
      utfOctet = 0;
      octetAvailable = true;
      for (cnt=0; cnt<BITS_PER_SYMB; cnt++)
        {
          if (ucsBits[cnt]>0)
            utfOctet += (1<<cnt);
        }
      // fprintf(stderr, " ((%d)) ", utfOctet);

      /* Decide, whether received octet is an idle symbol */
      if (utfOctet==IDLE_SYMB)
        {
          octetAvailable = false;
          rx_state->cntIdleSymbols++;
        }
      
      /* If more than MAX_IDLE_SYMB have been received or if more than */
      /* MAX_NUM_UNRELIABLE_GROSS_BITS gross bits with low reliability */
      /* have been received, assume that synchronization is lost and   */
      /* reset the wait_for_sync function.                             */
      if ((rx_state->cntIdleSymbols>= MAX_IDLE_SYMB) ||
          (rx_state->cntUnreliableGrossBits>MAX_NUM_UNRELIABLE_GROSS_BITS))
        {
          reinit_wait_for_sync(&(rx_state->wait_state));
          reinit_deinterleaver(&(rx_state->deintl_state));
          viterbi_reinit(&(rx_state->viterbi_state));
          rx_state->cntIdleSymbols = 0;
          rx_state->numDeintlBits = 0;
          
          /* pop all remaining bits from fifos and forget the bits */
          Shortint_fifo_reset(&(rx_state->net_bits_fifo_state));
          Shortint_fifo_reset(&(rx_state->rx_bits_fifo_state));
          
          octetAvailable = false;
#ifdef DEBUG_OUTPUT
          fprintf(stderr, ">>receiver offline<<");
#endif
        }
      
      /* If octet available -> push it into octet fifo buffer */
      if (octetAvailable)
        {
          Shortint_fifo_push(&(rx_state->octet_fifo_state), &utfOctet, 1);
          rx_state->cntIdleSymbols=0; /* reset counter for idle symbols */
        }
      
      /* Try to convert octets from buffer into UCS code.        */
      /* If successful, push decoded UCS code into output buffer */
      if (transformUTF2UCS(&ucsCode, &(rx_state->octet_fifo_state)))
        Shortint_fifo_push(ptr_output_char_fifo_state, &ucsCode, 1);
    }
}

