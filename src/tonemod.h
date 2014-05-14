/*
*******************************************************************************
*    
*
*      Changes since October 13, 2000:
*      - The type mod_state_t includes a new member txbits_fifo_state now.
*
*******************************************************************************
*
*      File             : tonemod.h
*      Purpose          : header file for tonemod.c
*
*******************************************************************************
*/

#ifndef tonemod_h
#define tonemod_h "$Id: $"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include <typedefs.h>
#include <fifo.h>

/*
*******************************************************************************
*                         DECLARATION OF PROTOTYPES
*******************************************************************************
*/


/* Define a type for the state variable of the function tonemod()     */

typedef struct {
  Shortint      cntModulatedSamples;
  Shortint      actualBits[2];
  fifo_state_t  txbits_fifo_state;
} mod_state_t;


/* ---------------------------------------------------------------------- */
/* Function init_tonemod()                                                */
/* ***********************                                                */
/* This function has to be executed before tonemod() can be used.         */
/* ---------------------------------------------------------------------- */

void init_tonemod(mod_state_t  *mod_state);


/* ---------------------------------------------------------------------- */
/* Function tonemod()                                                     */
/* ******************                                                     */
/* Modulator of the Cellular Text Telephone Modem.                        */
/* The input vector bits_in must contain the bits that have to be         */
/* transmitted. The length of bits_in must be even because always two     */
/* bits are coded in parallel.                                            */
/* Bits are either unipolar (i.e. {0, 1}) or bipolar (i.e. {-1, +1)}.     */
/* The length of the output vector tones_out must be 20 times longer than */
/* the length of bits_in, since each pair of two bits is coded within a   */
/* frame of 40 audio samples.                                             */
/* ---------------------------------------------------------------------- */

void tonemod(Shortint    *tones_out,
             Shortint    *bits_in,
             Shortint     num_samples_tones_out,
             Shortint     num_bits_in,
             mod_state_t *mod_state);

#endif
