/*
*******************************************************************************
*
*     
*
*******************************************************************************
*
*      File             : sin_fip.c
*      Purpose          : Fixed Point Sine Function
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/

#include "sin_fip.h"

#include <typedefs.h>
#include <stdlib.h>
#include <stdio.h>

const char sin_fip_id[] = "@(#)$Id$" sin_fip_h;

/*
*******************************************************************************
*                         PUBLIC PROGRAM CODE
*******************************************************************************
*/

/* ---------------------------------------------------------------------- */
/* sin_fip():                                                             */
/* Fixed Point sine function, returns the following value:                */
/* sin_fip(phase_value) = round(32767*sin(2*pi*50/8000*phase_value))      */
/* phase_value must be within the range [0...159].                        */
/* This function can be used for calculating sine waveforms of            */
/* frequencies that are integer-multiples of 50 Hz.                       */
/* ---------------------------------------------------------------------- */

Shortint sin_fip(Shortint phase_value)
{
  static const Shortint sine_table[] = {
    0,      1286,  2571,  3851,  5126,  6393,  7649,  8894, 10126, 11341, 
    12539, 13718, 14876, 16011, 17121, 18204, 19260, 20286, 21280, 22242, 
    23170, 24062, 24916, 25732, 26509, 27245, 27938, 28589, 29196, 29757, 
    30273, 30742, 31163, 31537, 31862, 32137, 32364, 32540, 32666, 32742, 
    32767, 32742, 32666, 32540, 32364, 32137, 31862, 31537, 31163, 30742, 
    30273, 29757, 29196, 28589, 27938, 27245, 26509, 25732, 24916, 24062, 
    23170, 22242, 21280, 20286, 19260, 18204, 17121, 16011, 14876, 13718, 
    12539, 11341, 10126,  8894,  7649,  6393,  5126,  3851,  2571,  1286};
  
  if ((phase_value>=160) && (phase_value<0))
    {
      fprintf(stderr, 
              "\nError in sin_fip(): phase value must be between 0 and 159\n");
      exit(1);
    }
  
  if (phase_value<80)
    return (sine_table[phase_value]);
  else
    return (-sine_table[phase_value-80]);
}

/* ---------------------------------------------------------------------- */
/* sin_fip2047():                                                         */
/* similar to sin_fip(), but different peak value:                        */
/* sin_fip(phase_value) = round(2047*sin(2*pi*50/8000*phase_value))       */
/* phase_value must be within the range [0...159].                        */
/* This function can be used for calculating sine waveforms of            */
/* frequencies that are integer-multiples of 50 Hz.                       */
/* ---------------------------------------------------------------------- */

Shortint sin_fip2047(Shortint phase_value)
{
  static const Shortint sine_table[] = {
    0,      80,  161,  241,  320,  399,  478,  556,
    633,   709,  783,  857,  929, 1000, 1070, 1137,
    1203, 1267, 1329, 1390, 1447, 1503, 1557, 1608,
    1656, 1702, 1745, 1786, 1824, 1859, 1891, 1920, 
    1947, 1970, 1990, 2008, 2022, 2033, 2041, 2045, 
    2047, 2045, 2041, 2033, 2022, 2008, 1990, 1970,
    1947, 1920, 1891, 1859, 1824, 1786, 1745, 1702,
    1656, 1608, 1557, 1503, 1447, 1390, 1329, 1267, 
    1203, 1137, 1070, 1000,  929,  857,  783,  709,  
    633,   556,  478,  399,  320,  241,  161,   80 };
  
  if ((phase_value>=160) && (phase_value<0))
    {
      fprintf(stderr, "\nError in sin_fip2047(): phase value must be between 0 and 159\n");
      exit(1);
    }
  
  if (phase_value<80)
    return (sine_table[phase_value]);
  else
    return (-sine_table[phase_value-80]);
}
