/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : conv_encoder.h
*      Purpose          : Header file for conv_encoder.c
*      Author           : Francisco Javier Gil Gomez
*
*******************************************************************************
*
*      Changes since October 13, 2000:
*      - added reset function conv_encoder_reset()
*
*******************************************************************************
* $Id: $ 
*
*/

#ifndef conv_encoder_h
#define conv_encoder_h "$Id: $"

#include "conv_poly.h"    /* definition of encoder_t */

/***********************************************************************/
/* conv_encoder_init()                                                 */
/* *******************                                                 */
/* Initialization of the convolutional encoder.                        */
/*                                                                     */
/* output variables:                                                   */
/* *ptr_state    Initialized state variable of the encoder             */
/***********************************************************************/

void conv_encoder_init(conv_encoder_t* ptr_state);

void conv_encoder_reset(conv_encoder_t* ptr_state);

/***********************************************************************/
/* conv_encoder_exec()                                                 */
/* *******************                                                 */
/* Execution of the convolutional encoder                              */
/*                                                                     */
/* input variables:                                                    */
/* in                   Vector with net bits                           */
/* inbits               Number of valid net bits in vector in.         */
/*                                                                     */
/* output variables:                                                   */
/* out                  Vector with the encoded gross bits. The gross  */
/*                      bits are either 0 or 1. The vector out must    */
/*                      have at least CHC_RATE*inbits elements.        */
/*                                                                     */
/* input/output variables:                                             */
/* *ptr_state           State variable of the encoder                  */
/*                                                                     */
/***********************************************************************/

void conv_encoder_exec(conv_encoder_t* ptr_state, 
                       Shortint* in,
                       Shortint  inbits, 
                       Shortint* out);

#endif
