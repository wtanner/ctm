/*
*******************************************************************************
*
*     
*
*******************************************************************************
*
*      File             : tonedemod.h
*      Purpose          : Demodulator for the Cellular Text Telephone Modem
*                         1-out-of-4 tones (400, 600, 800, 1000 Hz)
*                         for the coding of each pair of two adjacent bits
*
*                         Definition of the type demod_state_t and of the 
*                         functions init_tonedemod() and tonedemod()
*
*******************************************************************************
*/

#ifndef tonedemod_h
#define tonedemod_h "$Id: $"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include "ctm_defines.h"

#include <typedefs.h>

/*
*******************************************************************************
*                         DECLARATION OF PROTOTYPES
*******************************************************************************
*/

typedef struct {
  Shortint  buffer_tone_rx[3*SYMB_LEN];
  Shortint  xcorr_t0[2*SYMB_LEN];
  Shortint  xcorr_t1[2*SYMB_LEN];
  Shortint  xcorr_t2[2*SYMB_LEN];
  Shortint  xcorr_t3[2*SYMB_LEN];
  Shortint  xcorr_wb[2*SYMB_LEN];
  Shortint  lowpass[SYMB_LEN];
  Shortint  waveform_t0[SYMB_LEN];
  Shortint  waveform_t1[SYMB_LEN];
  Shortint  waveform_t2[SYMB_LEN];
  Shortint  waveform_t3[SYMB_LEN];
  Shortint  diff_smooth[SYMB_LEN];
} demod_state_t;


/* ----------------------------------------------------------------------- */
/* FUNCTION tonedemod()                                                    */
/* ********************                                                    */
/* Tone Demodulator for the Cellular Text Telephone Modem                  */
/* using one out of four tones for coding two bits in parallel within a    */
/* frame of 40 samples (5 ms).                                             */
/*                                                                         */
/* The function has to be called for every frame of 40 samples of the      */
/* received tone sequence. However, in order to track a non-ideal          */
/* of the transmitter's and the receiver's clock frequencies, one frame    */
/* might be shorter (only 39 samples) or longer (41 samples). The          */
/* of the following frame is indicated by the variable                     */
/* *sampling_correction, which is calculated and returned by this function.*/
/*                                                                         */
/* input variables:                                                         */
/* bits_out            contains the 39, 40 or 41 actual samples of the     */
/*                     received tones; the bits are soft bits, i.e. they   */
/*                     are in the range between -1.0 and 1.0, where the    */
/*                     magnitude serves as reliability information         */
/* num_in_samples      number of valid samples in bits_out                 */
/*                                                                         */
/* output variables:                                                        */
/* bits_out            contains the two actual decoded soft bits           */
/* sampling_correction is either -1, 0, or 1 and indicates whether the     */
/*                     next frame shall contain 39, 40, or 41 samples      */
/* demod_state         contains all the memory of tonedemod. Must be       */
/*                     initialized using the function init_tonedemod()     */
/* ----------------------------------------------------------------------- */

void tonedemod(Shortint *bits_out,
               Shortint *rx_tone_vec,
               Shortint num_in_samples,
               Shortint *ptr_sampling_correction,
               demod_state_t *demod_state);


/* ----------------------------------------------------------------------- */
/* FUNCTION init_tonedemod()                                               */
/* *************************                                               */
/* Initialization of one instance of the Tone Demodulator. The argument    */
/* must contain a pointer to a variable of type demod_state_t, which       */
/* contains all the memory of the tone demodulator. Each instance of       */
/* tonedemod must have its own variable.                                   */
/* ----------------------------------------------------------------------- */

void init_tonedemod(demod_state_t *demod_state);

#endif

