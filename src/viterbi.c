/*******************************************************************************
*
*******************************************************************************
*
*      File             : viterbi.c
*      Purpose          : Initialization & Execution of the Viterbi decoder
*      Author           : Francisco Javier Gil Gomez
*
*******************************************************************************
*
* $Id: $ 
*
*/

#include "ctm_defines.h"   /* CHC_RATE */
#include "conv_poly.h"
#include <typedefs.h>
#include <stdlib.h>
#include <stdio.h>


/**************************************************/
/* Forward declarations of locally used functions */
/**************************************************/

Shortint hamming_distance (Shortint a, Shortint b);
/* Returns the Hamming distance between two words of length = CHC_RATE    */
/* This function is used when a hard-decision decoding is performed       */

Longint distance (Shortint* analog, Shortint  binary);
/* This function is used instead 'hamming_distance' when a soft-decision  */
/* decoding is performed                                                  */

void butterfly (Shortint num, Shortint* inputvalue, node_t *nodes);
/* Calculates the new metrics of 4 paths in the trellis diagram at a time */
/* These four paths define a 'butterfly'                                  */


/***********************************************************************/
/* viterbi_init()                                                      */
/* **************                                                      */
/* Initialization of the Viterbi decoder.                              */
/*                                                                     */
/* output variables:                                                   */
/* *viterbi_state   initialized state variable of the decoder          */
/*                                                                     */
/***********************************************************************/

void viterbi_init (viterbi_t* viterbi_state)
{
  Shortint polya, polyb, polyc, polyd;
  Shortint out[CHC_RATE];   
  Shortint i,j,p,temp;
  
  /* Initialize number of steps */
  
  viterbi_state->number_of_steps = 0;
  
  /* Initialize nodes */
  
  for (i=0; i<NUM_NODES; i++)
    {
      viterbi_state->nodes[i].metric    = 0;
      viterbi_state->nodes[i].oldmetric = 0;
      for (p=0; p<BLOCK*CHC_K; p++)
        {
          viterbi_state->nodes[i].path[p] = -1;
          viterbi_state->nodes[i].temppath[p] = -1;
        }
    }
  
  /* Get the polynomials to be used */
  
  polynomials (CHC_RATE, CHC_K, &polya, &polyb, &polyc, &polyd);  
  
  /* Calculate base_output for each node */
  
  for (i=0; i < NUM_NODES/2; i++)
    {  
      temp = 2*i;
      
      out[0] = temp & polya;
      out[1] = temp & polyb;
      if (CHC_RATE>2)
        out[2] = temp & polyc;
      if (CHC_RATE>3)
        out[3] = temp & polyd;
      
      for (p=0; p<CHC_RATE; p++)
        {
          temp = 0;
          for (j=0; j<CHC_K-1; j++)
            {
              temp += (out[p] >> j) & 0x1;  
            }
          out[p] = temp%2;
        }
      
      temp = 0;
      for (p=0; p<CHC_RATE; p++)
        temp += (1<<p) * out[CHC_RATE-1-p];
      
      viterbi_state->nodes[2*i].base_output = temp;
      
      /* To calculate the base_output of the following node, */
      /* just invert the bits                                */
      
      viterbi_state->nodes[2*i+1].base_output = ~temp & ((1<<CHC_RATE)-1);
    }
}


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

void viterbi_reinit(viterbi_t* viterbi_state)
{
  Shortint i,p;
  
  /* Initialize number of steps */
  
  viterbi_state->number_of_steps = 0;
  
  /* Initialize nodes */
  
  for (i=0; i<NUM_NODES; i++)
    {
      viterbi_state->nodes[i].metric    = 0;
      viterbi_state->nodes[i].oldmetric = 0;
      for (p=0; p<BLOCK*CHC_K; p++)
        {
          viterbi_state->nodes[i].path[p] = -1;
          viterbi_state->nodes[i].temppath[p] = -1;
        }
    }
}

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

void  viterbi_exec(Shortint*  inputword, Shortint  length_input, 
                   Shortint*  out,       Shortint* num_valid_out_bits,
                   viterbi_t* viterbi_state)
{
  Shortint i,j,p;
  Longint biggest;
  Shortint min_metric;
  Shortint input_norm[CHC_RATE];
  Shortint groups;
  
  *num_valid_out_bits = 0;
  
  if (length_input % CHC_RATE != 0)
    {
      fprintf(stderr, "\nError in viterbi_exec():\n");
      fprintf(stderr, "length_input must be a multiple of CHC_RATE!\n");
      exit(1);
    }
  
  groups = length_input/CHC_RATE;
  
  for (j=0; j<groups; j++)
    {
      for (i=0; i<CHC_RATE; i++)
        input_norm[i] = inputword[CHC_RATE*j+i];
      
      /* Calculate the new metrics */
      
      for (i=0; i<NUM_NODES/2; i++)
        butterfly ((Shortint)(2*i), input_norm, viterbi_state->nodes);
      
      /* Update the metrics and the paths */
      
      for (i=0; i<NUM_NODES; i++)
        {
          viterbi_state->nodes[i].oldmetric = viterbi_state->nodes[i].metric;
          
          for (p=0; p<viterbi_state->number_of_steps; p++)
            viterbi_state->nodes[i].temppath[p] = viterbi_state->nodes[viterbi_state->nodes[i].continue_path_from].path[p];
          viterbi_state->nodes[i].temppath[viterbi_state->number_of_steps] = viterbi_state->nodes[i].new_entry;
        }
      
      /* Find the path with the lowest metric */
      
      biggest = maxLongint;
      min_metric = 0;
      
      for (i=0; i<NUM_NODES; i++)
        {
          for (p=0; p<BLOCK*CHC_K; p++)
            viterbi_state->nodes[i].path[p] 
              = viterbi_state->nodes[i].temppath[p];
          
          if (viterbi_state->nodes[i].metric < biggest)
            {
              biggest = viterbi_state->nodes[i].metric;
              min_metric = i;
            }
        }
      
      if (viterbi_state->number_of_steps >= BLOCK*CHC_K-1)
        {
          out[*num_valid_out_bits] = viterbi_state->nodes[min_metric].path[0];
          (*num_valid_out_bits)++;
          
          for (i=0; i<NUM_NODES; i++)
            for (p=0; p<BLOCK*CHC_K-1; p++)
              viterbi_state->nodes[i].path[p] 
                = viterbi_state->nodes[i].path[p+1];
        }
      else 
        viterbi_state->number_of_steps ++;
    }  
}


Shortint hamming_distance (Shortint a, Shortint b)
{
  Shortint exor;
  Shortint cnt;
  Shortint tmp;
  
  a &= (1<<CHC_RATE)-1;
  b &= (1<<CHC_RATE)-1;
  tmp = 0;
  exor = a ^ b;
  for (cnt=0; cnt<CHC_RATE; cnt++)
    tmp += (exor >> cnt) & 0x1;
  return (tmp);
}

Longint distance (Shortint * analog, Shortint  binary)
{
  /* Example:  analog = [13423,22432,-12555] binary = [1,1,0] */
  Shortint ii;
  Shortint temp;
  Longint  dist;
  Shortint analog_tmp;
  
  temp = 0;
  dist = 0;
  
  binary &= (1<<CHC_RATE)-1;
  
  /* We transform 0 -> -16383, 1 -> 16383 */
  for (ii=0; ii<CHC_RATE; ii++)
    {  
      temp = (binary >> ii) & 0x1;
      if (temp == 0)
        temp = -16383;
      else if (temp == 1)
        temp = 16383;
      else
        {
          fprintf(stderr, "\nError: Error calculating the path metrics.\n");
          exit(1);
        }
      analog_tmp = analog[CHC_RATE-1-ii];
      
      if (analog_tmp > 16383)
        analog_tmp = 16383;
      else if (analog_tmp < -16383)
        analog_tmp = -16383;
      
      dist += abs(temp-analog_tmp);
    }
  return(dist);
}


void butterfly (Shortint num, Shortint* inputvalue, node_t *nodes)
{
  Longint my_metric         = nodes[num].oldmetric;
  Longint my_friends_metric = nodes[num+1].oldmetric;
  Longint path0, path1;
  
  path0 = my_metric         + distance(inputvalue, nodes[num].base_output);  
  path1 = my_friends_metric + distance(inputvalue, nodes[num+1].base_output);
  
  if (path0>path1)
    {
      nodes[num/2].metric = path1;
      nodes[num/2].continue_path_from = num+1;
    }
  else
    {
      nodes[num/2].metric = path0;
      nodes[num/2].continue_path_from = num;
    }
  nodes[num/2].new_entry = 0;
  
  path0 = (my_metric         
           + distance(inputvalue, (Shortint)(~nodes[num].base_output)));  
  path1 = (my_friends_metric 
           + distance(inputvalue, (Shortint)(~nodes[num+1].base_output)));
  
  if (path0>path1)
    {
      nodes[num/2+NUM_NODES/2].metric = path1;
      nodes[num/2+NUM_NODES/2].continue_path_from = num+1;
    }
  else
    {
      nodes[num/2+NUM_NODES/2].metric = path0;
      nodes[num/2+NUM_NODES/2].continue_path_from = num;
    }
  nodes[num/2+NUM_NODES/2].new_entry = 1;
}

