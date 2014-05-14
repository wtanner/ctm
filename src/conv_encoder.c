/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : conv_encoder.c
*      Purpose          : Convolutional encoder.
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

#include "ctm_defines.h"   /* CHC_RATE */
#include "conv_poly.h"
#include <typedefs.h>

#include <stdio.h>


/***********************************************************************/
/* conv_encoder_init()                                                 */
/* *******************                                                 */
/* Initialization of the convolutional encoder.                        */
/*                                                                     */
/* output variables:                                                   */
/* *ptr_state    Initialized state variable of the encoder             */
/***********************************************************************/

void conv_encoder_init(conv_encoder_t* ptr_state)
{
  Shortint  polya, polyb, polyc, polyd;
  Shortint  poly [CHC_RATE];
  Shortint  cnt,i,j;
  
  /* Get the polynomials for the desired parameters */
  
  polynomials (CHC_RATE, CHC_K, &polya, &polyb, &polyc, &polyd);
  poly[0] = polya;
  poly[1] = polyb;
  if (CHC_RATE > 2)
    {
      poly[2] = polyc;
      if (CHC_RATE > 3)
        poly[3] = polyd;
    }
  
  /* Generate the impulse response */
  
  cnt=0;
  for (i=CHC_K; i>0; i--)
    for (j=0; j<CHC_RATE; j++)
      {
        ptr_state->impulse_response[cnt] = (poly[j] >> (i-1)) & 0x1;
        cnt++;
      }
  
  /* Reset the temp variable */
  
  for (i=0; i<CHC_RATE*CHC_K; i++)
    ptr_state->temp[i] = 0;
}


void conv_encoder_reset(conv_encoder_t* ptr_state)
{
  Shortint i;
  
  /* Reset the temp variable */
  for (i=0; i<CHC_RATE*CHC_K; i++)
    ptr_state->temp[i] = 0;
}


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
                       Shortint* out)
{
  Shortint i,p;
  Shortint input;
  
  for (p=0; p<inbits; p++)
    {
      /* Make shure that net bits are either 0 or 1 */
      input = (in[p]>0) ? 1 : 0;
      
      for (i=0; i<CHC_RATE*CHC_K; i++)
        {
          ptr_state->temp[i] += input * ptr_state->impulse_response[i];
          ptr_state->temp[i] %= 2;
        }
      
      /* For each input bit we have CHC_RATE output bits */
      for (i=0; i<CHC_RATE; i++)
        out[i + CHC_RATE*p] = ptr_state->temp[i];
      
      
      for (i=0; i<CHC_RATE*CHC_K; i++)
        ptr_state->temp[i] = ptr_state->temp[i+CHC_RATE];
      
      for (i=0; i<CHC_RATE; i++)
        ptr_state->temp[CHC_K*CHC_RATE-i-1] = 0;
    }
}

