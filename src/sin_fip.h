/*
*******************************************************************************
*
*     
*
*******************************************************************************
*
*      File             : sin_fip.h
*      Purpose          : Fixed Point Sine Function
*
*******************************************************************************
*/
#ifndef sin_fip_h
#define sin_fip_h "$Id$"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include <typedefs.h>

/*
*******************************************************************************
*                         DECLARATION OF PROTOTYPES
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

Shortint sin_fip(Shortint phase_value);

/* ---------------------------------------------------------------------- */
/* sin_fip2047():                                                         */
/* similar to sin_fip(), but with different peak value:                   */
/* sin_fip(phase_value) = round(2047*sin(2*pi*50/8000*phase_value))       */
/* phase_value must be within the range [0...159].                        */
/* This function can be used for calculating sine waveforms of            */
/* frequencies that are integer-multiples of 50 Hz.                       */
/* ---------------------------------------------------------------------- */

Shortint sin_fip2047(Shortint phase_value);

#endif
