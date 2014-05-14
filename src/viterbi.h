/*
*******************************************************************************
*
*      
*
     File             : viterbi.h
*      Purpose          : Header file for viterbi.c
*      Author           : Francisco Javier Gil Gomez
*
*      Revision history
*
*      $Log: $
*
*******************************************************************************
*/

#ifndef viterbi_h
#define viterbi_h "$Id: $"

#include <stdio.h>
#include <typedefs.h>
#include <math.h>

#include "conv_poly.h"   /* definition of viterbi_t */


/***********************************************************************/
/* viterbi_init()                                                      */
/* **************                                                      */
/* Initialization of the Viterbi decoder.                              */
/*                                                                     */
/* output variables:                                                   */
/* *viterbi_state   initialized state variable of the decoder          */
/*                                                                     */
/***********************************************************************/

void viterbi_init(viterbi_t* viterbi_state);


/***********************************************************************/
/* viterbi_reinit()                                                    */
/* ****************                                                    */
/* Re-Initialization of the Viterbi decoder. This function should be   */
/* used for re-setting a Viterbi decoder that has already been         */
/* initialized. In contrast to viterbi_init(), this reinit function    */
/* does not calculate the values of all members of viterbi_state that  */
/* do not change during the execution of the Viterbi algorithm.        */
/*                                                                     */
/* output variables:                                                   */
/* *viterbi_state   initialized state variable of the decoder          */
/*                                                                     */
/***********************************************************************/

void viterbi_reinit(viterbi_t* viterbi_state);


/***********************************************************************/
/* viterbi_exec()                                                      */
/* **************                                                      */
/* Execution of the Viterbi decoder                                    */
/*                                                                     */
/* input variables:                                                    */
/* inputword            Vector with gross bits                         */
/* length_input         Number of valid gross bits in vector inputword.*/
/*                      length_input must be an integer multiple of    */
/*                      CHC_RATE                                       */
/*                                                                     */
/* output variables:                                                   */
/* out                  Vector with the decoded net bits. The net bits */
/*                      are either 0 or 1.                             */
/* *num_valid_out_bits  Number of valid bits in vector out             */
/*                                                                     */
/* input/output variables:                                             */
/* *viterbi_state       State variable of the decoder                  */
/*                                                                     */
/***********************************************************************/

void viterbi_exec(Shortint*  inputword, Shortint  length_input, 
                  Shortint*  out,       Shortint* num_valid_out_bits,
                  viterbi_t* viterbi_state);

#endif
