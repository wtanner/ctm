/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : ctm_transmitter.h
*      Author           : EEDN/RV Matthias Doerbecker
*      Tested Platforms : Sun Solaris, Windows NT 4.0
*      Description      : header file for ctm_transmitter.c
*
*      Changes since October 13, 2000:
*      - added reset function reset_ctm_transmitter()
*
*      $Log: $
*
*******************************************************************************
*/
#ifndef ctm_transmitter_h
#define ctm_transmitter_h "$Id: $"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include "ctm_defines.h"
#include "init_interleaver.h"
#include "tonemod.h"
#include "conv_poly.h"

#include <typedefs.h>
#include <fifo.h>

/* ******************************************************************/
/* Type definitions for variables that contain all states of the    */
/* CTM Transmitter                                                  */
/* ******************************************************************/

typedef struct {
  Bool                 burstActive;
  Shortint             cntIdleSymbols;
  Shortint             cntEncBitsInCurrentBlock;
  Shortint             cntTXBitsInCurrentBlock;

  /* vectors (not to be allocated) */
  Shortint             resync_sequence[RESYNC_SEQ_LENGTH];
  
#if (NUM_MUTE_ROWS>0)
  Shortint              mutePositions[intlvB*NUM_MUTE_ROWS];
#else
  Shortint              mutePositions[1];
#endif
  
  fifo_state_t         fifo_state;
  fifo_state_t         octet_fifo_state;
  interleaver_state_t  diag_int_state;
  mod_state_t          mod_state;
  conv_encoder_t       conv_enc_state;
} tx_state_t;


/***********************************************************************/
/* init_ctm_transmitter()                                              */
/* **********************                                              */
/* Initialization of the CTM Transmitter                               */
/*                                                                     */
/* output vaiables:                                                    */
/* tx_state :   pointer to a variable of tx_state_t containing the     */
/*              initialized states of the receiver                     */
/***********************************************************************/

void init_ctm_transmitter(tx_state_t* tx_state);

void reset_ctm_transmitter(tx_state_t* tx_state);


/***********************************************************************/
/* ctm_transmitter()                                                   */
/* *****************                                                   */
/* Runs the Cellular Text Telephone Modem (CTM) Transmitter for a      */
/* block of 160 output samples, representing 8 gross bits.             */
/* The bits, which are modulated into tones, are taken from an         */
/* internal fifo buffer. If the fifo buffer is empty, zero-valued      */
/* samples are generated. The fifo buffer is filled with               */
/* channel-encoded and interlaeved bits, which are generated           */
/* internally by coding the actual input character.  With each call of */
/* this function one or less input characters can be coded. If there   */
/* is no character to for transmission, one of the following code has  */
/* be used:                                                            */
/* - 0xFFFF:  indicates that there is no character to transmit and     */
/*            that the transmitter should stay in idle mode, if it is  */
/*            currently already in idle mode. If the transmitter is    */
/*            NOT in idle mode, it might generate <IDLE> symbols in    */
/*            to keep an active burst running. The CTM burst is        */
/*            terminated if five <IDLE> symbols have been generated    */
/*            consecutevely.                                           */
/*                                                                     */
/* - 0xFFFE:  although there is no character to transmit, a CTM burst  */
/*            is initiated in order to signalize to the far-end side   */
/*            that CTM is supported. The burst starts with the <IDLE>  */
/*            symbol and will be contiued with <IDLE> symbols if there */
/*            are no regular characters handed over during the next    */
/*            calls of this function. The CTM burst is terminated if   */
/*            five <IDLE> symbols have been transmitted consecutevely. */
/*                                                                     */
/* In order to avoid an overflow of the internal fifo buffer, the      */
/* variable *ptrNumBitsStillToModulate should be checked before        */
/* calling this function.                                              */
/*                                                                     */
/* input variables:                                                    */
/* ucsCode                    UCS code of the character                */
/*                            or one of the codes 0xFFFF or 0xFFFE     */
/* sineOutput                 must be false in regular mode; if true,  */
/*                            a pure sine output signal is generated   */
/* output variables:                                                   */
/* txToneVec                  output signal (vector  of 160 samples)   */
/* input/output variables:                                             */
/* tx_state                   pointer to the variable containing the   */
/*                            transmitter states                       */
/***********************************************************************/

void ctm_transmitter(UShortint   ucsCode, 
                     Shortint*   txToneVec,
                     tx_state_t* tx_state, 
                     Shortint   *ptrNumBitsStillToModulate,
                     Bool        sineOutput);

#endif
