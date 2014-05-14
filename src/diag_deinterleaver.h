/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : diag_interleaver.h
*      Purpose          : diagonal (chain) deinterleaver routine
*
*******************************************************************************
*/
#ifndef diag_deinterleaver_h
#define diag_deinterleaver_h "$Id: $"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include "init_interleaver.h"

#include <typedefs.h>

/*
*******************************************************************************
*                         DECLARATION OF PROTOTYPES
*******************************************************************************
*/

/* ---------------------------------------------------------------------- */
/* diag_deinterleaver:                                                    */
/* Corresponding deinterleaver to diag_interleaver.                       */
/* An arbitrary number of bits can be interleaved, depending of the       */
/* length of the vector "in". The vector "out", which must have the same  */
/* length than "in", contains the interleaved samples.                    */
/* All states (memory etc.) of the interleaver are stored in the variable */
/* *intl_state. Therefore, a pointer to this variable must be handled to  */
/* this function. This variable initially has to be initialized by the    */
/* function init_interleaver, which offers also the possibility to        */
/* specify the dimensions of the deinterleaver matrix.                    */
/* ---------------------------------------------------------------------- */

void diag_deinterleaver(Shortint *out,
                        Shortint *in,
                        Shortint num_valid_bits,
                        interleaver_state_t *intl_state);

/* ---------------------------------------------------------------------- */
/* shift_deinterleaver:                                                   */
/* Shift of the deinterleaver buffer by <shift> samples.                  */
/* shift>0  -> shift to the right                                         */
/* shift<0  -> shift to the left                                          */
/* The elements from <insert_bits> are inserted into the resulting space. */
/* The vector <insert_bits> must have at least abs(shift) elements.       */
/* ---------------------------------------------------------------------- */

void shift_deinterleaver(Shortint shift,
                         Shortint *insert_bits,
                         interleaver_state_t *ptr_state);


#endif
