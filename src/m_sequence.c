/*
*******************************************************************************
*
*     
*
*******************************************************************************
*
*      File             : m_sequence.c
*      Purpose          : Calculation of m-sequences 
*                         (maximum-length sequecnes or pseudo noise)
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/

#include "m_sequence.h"

#include <typedefs.h>

#include <stdio.h>    

const char m_sequence_id[] = "@(#)$Id: $" m_sequence_h;

/*
*******************************************************************************
*                         PUBLIC PROGRAM CODE
*******************************************************************************
*/

void m_sequence(Shortint *sequence, Shortint length)
{
  Shortint  r, cnt, cnt2, filter_output;
  Shortint  s_vec[10] = {0,0,0,0,0,0,0,0,0,0};
  
  /* Matrix with primitive polynoms in GF(2).              */
  /* Each row of this table contains the coefficients      */
  /* a1, a2, a3, ..., a10. The coefficient a0 is always 1. */
  const Shortint a_matrix[9][10] = {
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},   /* r=2  */
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0},   /* r=3  */
    {1, 0, 0, 1, 0, 0, 0, 0, 0, 0},   /* r=4  */
    {0, 1, 0, 0, 1, 0, 0, 0, 0, 0},   /* r=5  */
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0},   /* r=6  */
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0},   /* r=7  */
    {1, 0, 0, 0, 1, 1, 0, 1, 0, 0},   /* r=8  */
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0},   /* r=9  */
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 1}    /* r=10 */
  };
  
  /* Check whether the sequence length fits to */
  /* 2^r-1 and determine the root r            */
  r=0;
  for (cnt=2; cnt<=10; cnt++)
    if ((1<<cnt)-1 == length)
      r=cnt;
  
  if (r==0)
    {
      fprintf(stderr, "\nFunction m_sequence:\n");
      fprintf(stderr, "Invalid sequence length; must be 2^r-1 with 2<=r<=10\n\n");
      exit(1);
    }
  
  for (cnt=0; cnt<length; cnt++)
    {
      if (cnt==0)
        sequence[cnt] = 1;
      else
        {
          filter_output = 0;
          for (cnt2=0; cnt2<r; cnt2++)
            {
              filter_output = filter_output+s_vec[cnt2]*a_matrix[r-2][cnt2];
            }
          sequence[cnt] = 1 - (filter_output % 2);
        }
      for (cnt2=r-1; cnt2>0; cnt2--)
        s_vec[cnt2] = s_vec[cnt2-1];
      s_vec[0] = sequence[cnt];
    }
  
  /* transform unipolar sequence into bipolar */
  for (cnt=0; cnt<length; cnt++)
    sequence[cnt] = 1-2*sequence[cnt];
}

