/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : tonedemod.c
*      Purpose          : Demodulator for the Cellular Text Telephone Modem
*                         1-out-of-4 tones (400, 600, 800, 1000 Hz)
*                         for the coding of each pair of two adjacent bits
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/

#include "tonedemod.h"
#include "ctm_defines.h"
#include "sin_fip.h"

#include <typedefs.h>

#include <stdio.h>    

const char tonedemod_id[] = "@(#)$Id: $" tonedemod_h;

/*
*******************************************************************************
*              PRIVATE PROGRAM CODE AND VARIABLES
*******************************************************************************
*/

void rotate_right(Shortint *samples)
{
  Shortint  cnt;
  Shortint  tmp_value;
  
  tmp_value = samples[SYMB_LEN-1];
  for (cnt=SYMB_LEN-1; cnt>0; cnt--)
    samples[cnt] = samples[cnt-1];
  samples[0]=tmp_value;
}

void rotate_left(Shortint *samples)
{
  Shortint  cnt;
  Shortint  tmp_value;
  
  tmp_value = samples[0];
  for (cnt=0; cnt<SYMB_LEN-1; cnt++)
    samples[cnt] = samples[cnt+1];
  samples[SYMB_LEN-1]=tmp_value;
}


/*
*******************************************************************************
*                         PUBLIC PROGRAM CODE
*******************************************************************************
*/
void init_tonedemod(demod_state_t *demod_state)
{
  /* Table with the sinc function (0<=cnt<SYMB_LEN)             */
  /* (floor(0.5+32767*sin(2*pi*(cnt-SYMB_LEN/2+1)/SYMB_LEN)/    */
  /*                     (2*pi*(cnt-SYMB_LEN/2+1)/SYMB_LEN)))   */
#if SYMB_LEN==40
  static const Shortint sinc_window[] = {
    1717,   3581,   5571,   7663,   9834,  
    12054,  14297,  16533,  18730,  20860,  
    22893,  24799,  26552,  28127,  29501,  
    30653,  31568,  32231,  32632,  32767,  
    32632,  32231,  31568,  30653,  29501, 
    28127,  26552,  24799,  22893,  20860,
    18730,  16533,  14297,  12054,   9834, 
    7663,   5571,   3581,   1717,      0};
#endif
#if SYMB_LEN==32
  static const Shortint sinc_window[] = {
    2170,   4562,    7132,   9834,   12614,
    15418,  18186,  20860,  23382,   25696,  
    27751,  29501,  30905,  31931,   32557,    
    32767,  32557,  31931,  30905,   29501, 
    27751,  25696,  23382,  20860,   18186, 
    15418,  12614,   9834,   7132,    4562,
    2170,     0};
#endif
  
  Shortint cnt;
  Longint  sum_value;
  
  for (cnt=0 ; cnt<SYMB_LEN ; cnt++)
    {
      demod_state->waveform_t0[cnt] 
        = sin_fip((Shortint)(((160/SYMB_LEN)*cnt*NCYCLES_0)%160))/SYMB_LEN;
      demod_state->waveform_t1[cnt] 
        = sin_fip((Shortint)(((160/SYMB_LEN)*cnt*NCYCLES_1)%160))/SYMB_LEN;
      demod_state->waveform_t2[cnt] 
        = sin_fip((Shortint)(((160/SYMB_LEN)*cnt*NCYCLES_2)%160))/SYMB_LEN;
      demod_state->waveform_t3[cnt] 
        = sin_fip((Shortint)(((160/SYMB_LEN)*cnt*NCYCLES_3)%160))/SYMB_LEN;

      demod_state->diff_smooth[cnt] = 0;
    }
  for (cnt=0 ; cnt<2*SYMB_LEN ; cnt++)
    {
      demod_state->xcorr_t0[cnt] = 0;
      demod_state->xcorr_t1[cnt] = 0;
      demod_state->xcorr_t2[cnt] = 0;
      demod_state->xcorr_t3[cnt] = 0;
      demod_state->xcorr_wb[cnt] = 0;
    }
  for (cnt=0 ; cnt<3*SYMB_LEN ; cnt++)
    demod_state->buffer_tone_rx[cnt] = 0;
  
  sum_value = 0;
  for (cnt=0 ; cnt<SYMB_LEN ; cnt++)
    {
      demod_state->lowpass[cnt] = sinc_window[cnt];
      sum_value    = sum_value + demod_state->lowpass[cnt];
    }
  
  /* The lowpass impulse response is normalized to the sum of its */
  /* coefficients, resulting in a frequency response of 32767 (0 dB)  */
  /* for low frequencies                                          */
  for (cnt=0 ; cnt<SYMB_LEN ; cnt++)
    demod_state->lowpass[cnt] = 
      (Shortint)(((Longint)(demod_state->lowpass[cnt])*32767)/sum_value);
}

/* ---------------------------------------------------------------------- */  

void tonedemod(Shortint *bits_out,
               Shortint *in_samples,
               Shortint num_in_samples,
               Shortint *ptr_sampling_correction,
               demod_state_t *demod_state)
{
  static const Longint alpha           = 32113; /* = 32768*0.98 */
  static const Longint one_minus_alpha = 655;   /* = 32768*0.02 */
  static const Longint alpha2          = 32440; /* = 32768*0.99 */
  
  Longint sum0, sum1, sum2, sum3, sumw;
  
  Shortint  cnt, lag, index_max;
  Shortint  gain;
  Shortint  max_diff;
  Shortint  max_diff_smooth;
  Shortint  soft_value;
  Shortint  xcorr0, xcorr1, xcorr2, xcorr3, xcorrw;
  
  Shortint  xcorr_abs_t0[2*SYMB_LEN];
  Shortint  xcorr_abs_t1[2*SYMB_LEN];
  Shortint  xcorr_abs_t2[2*SYMB_LEN];
  Shortint  xcorr_abs_t3[2*SYMB_LEN];
  Shortint  xcorr_abs_wb[2*SYMB_LEN];

  Shortint  xcorr_lp_t0[SYMB_LEN];
  Shortint  xcorr_lp_t1[SYMB_LEN];
  Shortint  xcorr_lp_t2[SYMB_LEN];
  Shortint  xcorr_lp_t3[SYMB_LEN];
  Shortint  xcorr_lp_wb[SYMB_LEN];
  Shortint  diff[SYMB_LEN];
  
  /* ################################################################### */
  /* ############### The following is for debugging only ############### */
  /* #############  Open two files with debug information ############## */
  /* ################################################################### */
  
#ifdef DEBUG_OUTPUT
  static Bool    firsttime=true;
  static FILE    *out_file1, *out_file2;
  double         dbl_value;
  
  if (firsttime)
    {
      firsttime = false;
      
      if ((out_file1=fopen("debug_info1.dbl", "wb"))==NULL)
        {
          fprintf(stderr, "Error while opening debug_info1.dbl\n\n") ;
          exit(1);
        }
      if ((out_file2=fopen("debug_info2.dbl", "wb"))==NULL)
        {
          fprintf(stderr, "Error while opening debug_info2.dbl\n\n") ;
          exit(1);
        }
    } 
#endif  
  
  /* ################################################################### */
  /* ################################################################### */
  
  /* The regular framelength is SYMB_LEN samples.                     */
  /* If the actual framelength is SYMB_LEN+1 or SYMB_LEN-1 samples,   */
  /* the buffer demod_state->diff_smooth must be shifted accordingly  */
  
  switch (num_in_samples) {
  case SYMB_LEN-1:
    rotate_right(demod_state->diff_smooth);
    break;
  case SYMB_LEN+1:
    rotate_left(demod_state->diff_smooth);
    break;
  case SYMB_LEN:
    /* do nothing special */
    break;
  default:
    fprintf(stderr, "tonedemod: Invalid value for num_in_samples!\n");
    exit(1);
  }
  
  /* Read in the actual input samples. This can be either                 */
  /* SYMB_LEN, SYMB_LEN+1, or SYMB_LEN-1. In order to make the            */
  /* remaining code of this function independent of the number of input   */
  /* samples, an input-buffer is used, which is shifted according to      */
  /* the number of the new samples                                        */
  
  for (cnt=0; cnt<3*SYMB_LEN-num_in_samples; cnt++)
    demod_state->buffer_tone_rx[cnt] 
      = demod_state->buffer_tone_rx[cnt+num_in_samples];
  
  for (cnt=0; cnt<num_in_samples; cnt++)
    demod_state->buffer_tone_rx[cnt+3*SYMB_LEN-num_in_samples] 
      = in_samples[cnt];
  
  /* Now calculate the cross-correlations. For each cross-correlation     */
  /* more than SYMB_LEN samples have to be caluclated because a lowpass   */
  /* filtering shall be applied in the next step.                         */
  
  /* Since the input buffer has been shifted by num_in_samples, the       */
  /* first SYMB_LEN-1 correlation values can be obtained by copying       */
  /* the appropriate values from the last frame.                          */
  
  for (lag=0; lag<SYMB_LEN-1; lag++)
    {
      demod_state->xcorr_t0[lag] = demod_state->xcorr_t0[lag+num_in_samples];
      demod_state->xcorr_t1[lag] = demod_state->xcorr_t1[lag+num_in_samples];
      demod_state->xcorr_t2[lag] = demod_state->xcorr_t2[lag+num_in_samples];
      demod_state->xcorr_t3[lag] = demod_state->xcorr_t3[lag+num_in_samples];
      demod_state->xcorr_wb[lag] = demod_state->xcorr_wb[lag+num_in_samples];
    } 
  
  /* Calculate the remaining correlation values. */
  
  for (lag=SYMB_LEN-1; lag<2*SYMB_LEN; lag++)
    {
      sum0 = 0L;
      sum1 = 0L;
      sum2 = 0L;
      sum3 = 0L;
      sumw = 0L;
      
      for (cnt=0; cnt<SYMB_LEN; cnt++)
        {
          sum0 += ((Longint)demod_state->buffer_tone_rx[lag+cnt]*
                   (Longint)demod_state->waveform_t0[cnt]);
          sum1 += ((Longint)demod_state->buffer_tone_rx[lag+cnt]*
                   (Longint)demod_state->waveform_t1[cnt]);
          sum2 += ((Longint)demod_state->buffer_tone_rx[lag+cnt]*
                   (Longint)demod_state->waveform_t2[cnt]);
          sum3 += ((Longint)demod_state->buffer_tone_rx[lag+cnt]*
                   (Longint)demod_state->waveform_t3[cnt]);
          sumw += ((Longint)abs(demod_state->buffer_tone_rx[lag+cnt]));
        }
      demod_state->xcorr_t0[lag] =  sum0>>15;
      demod_state->xcorr_t1[lag] =  sum1>>15;
      demod_state->xcorr_t2[lag] =  sum2>>15;
      demod_state->xcorr_t3[lag] =  sum3>>15;
      demod_state->xcorr_wb[lag] =  sumw/SYMB_LEN;
    } 
  
  for (lag=0; lag<2*SYMB_LEN; lag++)
    {
      xcorr_abs_t0[lag] = abs(demod_state->xcorr_t0[lag]);
      xcorr_abs_t1[lag] = abs(demod_state->xcorr_t1[lag]);
      xcorr_abs_t2[lag] = abs(demod_state->xcorr_t2[lag]);
      xcorr_abs_t3[lag] = abs(demod_state->xcorr_t3[lag]);
      xcorr_abs_wb[lag] = abs(demod_state->xcorr_wb[lag]);
    }
  
  /* Calculate the low-pass filtered cross-correlations.       */
  for (lag=0; lag<SYMB_LEN; lag++)
    {
      sum0 = 0L;
      sum1 = 0L;
      sum2 = 0L;
      sum3 = 0L;
      sumw = 0L;
      for (cnt=0; cnt<SYMB_LEN; cnt++)
        {
          sum0 += ((Longint)xcorr_abs_t0[SYMB_LEN+lag-cnt]*
                   (Longint)demod_state->lowpass[cnt]);
          sum1 += ((Longint)xcorr_abs_t1[SYMB_LEN+lag-cnt]*
                   (Longint)demod_state->lowpass[cnt]);
          sum2 += ((Longint)xcorr_abs_t2[SYMB_LEN+lag-cnt]*
                   (Longint)demod_state->lowpass[cnt]);
          sum3 += ((Longint)xcorr_abs_t3[SYMB_LEN+lag-cnt]*
                   (Longint)demod_state->lowpass[cnt]);
          sumw += ((Longint)xcorr_abs_wb[SYMB_LEN+lag-cnt]*
                   (Longint)demod_state->lowpass[cnt]);
        }
      xcorr_lp_t0[lag] = sum0>>15;
      xcorr_lp_t1[lag] = sum1>>15;
      xcorr_lp_t2[lag] = sum2>>15;
      xcorr_lp_t3[lag] = sum3>>15;
      xcorr_lp_wb[lag] = sumw>>15;
    }
  
  /* Calculate the sum of all possible differences between the */
  /* low-pass-filtered correlations.                           */
  max_diff = 0;
  for (lag=0; lag<SYMB_LEN; lag++)
    {
      sum0 = (labs((Longint)xcorr_lp_t0[lag]-(Longint)xcorr_lp_t1[lag]) +
              labs((Longint)xcorr_lp_t0[lag]-(Longint)xcorr_lp_t2[lag]) +
              labs((Longint)xcorr_lp_t0[lag]-(Longint)xcorr_lp_t3[lag]) +
              labs((Longint)xcorr_lp_t1[lag]-(Longint)xcorr_lp_t2[lag]) +
              labs((Longint)xcorr_lp_t1[lag]-(Longint)xcorr_lp_t3[lag]) +
              labs((Longint)xcorr_lp_t2[lag]-(Longint)xcorr_lp_t3[lag]));
      diff[lag] = (Shortint)(sum0/6);
      if (diff[lag]>max_diff)
        max_diff = diff[lag];
    }
  
  /* In order to improve the performance of the following IIR filter,   */
  /* an adaptive gain factor of 2^(gain) is applied to the vector diff. */
  
  if (max_diff<2048)
    gain=4;
  else if (max_diff<4096)
    gain=3;
  else if (max_diff<8192)
    gain=2;
  else if (max_diff<16384)
    gain=1;
  else
    gain=0;
  
  /* Update the smoothed difference */
  for (lag=0; lag<SYMB_LEN; lag++)
    if (max_diff > 4) 
      demod_state->diff_smooth[lag] 
        = (Shortint)((alpha*(Longint)((demod_state->diff_smooth[lag])) +
                      one_minus_alpha*(Longint)(diff[lag]<<gain))>>15);
    else
      demod_state->diff_smooth[lag] 
        = (Shortint)((alpha2*(Longint)(demod_state->diff_smooth[lag]))>>15);
  
  /* Search the maximum of the smoothed difference */
  index_max = 0;
  max_diff_smooth = 0;
  for (lag=0; lag<SYMB_LEN; lag++)
    if (demod_state->diff_smooth[lag] > max_diff_smooth)
      {
        max_diff_smooth = demod_state->diff_smooth[lag];
        index_max       = lag;
      }

  /* Calculate the soft bits from the cross-correlations */
  /* at the index that has been determined previously    */ 
  xcorr0 = xcorr_lp_t0[index_max];
  xcorr1 = xcorr_lp_t1[index_max];
  xcorr2 = xcorr_lp_t2[index_max];
  xcorr3 = xcorr_lp_t3[index_max];
  xcorrw = xcorr_lp_wb[index_max];
  
  if      ((xcorr0 >= xcorr1) && (xcorr0 >= xcorr2) && (xcorr0 >= xcorr3))
    {
      soft_value = 
        xcorr0-(Shortint)(((Longint)xcorr1+(Longint)xcorr2+(Longint)xcorr3)/3);
      bits_out[0] = -soft_value;
      bits_out[1] = -soft_value;
    }
  else if ((xcorr1 >= xcorr0) && (xcorr1 >= xcorr2) && (xcorr1 >= xcorr3))
    {
      soft_value = 
        xcorr1-(Shortint)(((Longint)xcorr0+(Longint)xcorr2+(Longint)xcorr3)/3);
      bits_out[0] = -soft_value;
      bits_out[1] =  soft_value;
    }
  else if ((xcorr2 >= xcorr0) && (xcorr2 >= xcorr1) && (xcorr2 >= xcorr3))
    {
      soft_value = 
        xcorr2-(Shortint)(((Longint)xcorr0+(Longint)xcorr1+(Longint)xcorr3)/3);
      bits_out[0] =  soft_value;
      bits_out[1] = -soft_value;
    }
  else
    {
      soft_value = 
        xcorr3-(Shortint)(((Longint)xcorr0+(Longint)xcorr1+(Longint)xcorr2)/3);
      bits_out[0] =  soft_value;
      bits_out[1] =  soft_value;
    }
  
  if (7L*(Longint)soft_value > (Longint)(xcorrw+10))
    {
      bits_out[0] = (bits_out[0] | 0x0001);
      bits_out[1] = (bits_out[1] | 0x0001);
    }
  else
    {
      bits_out[0] = (bits_out[0] & 0xFFFE);
      bits_out[1] = (bits_out[1] & 0xFFFE);
    }

  /* Calculate the sampling_correction for the next frame. */
  /* This correction is either -1, 0, or +1.               */
  *ptr_sampling_correction = 0;
  
  if (max_diff>40)
    {
      if (index_max < SYMB_LEN/2)
        *ptr_sampling_correction = -1;
      
      if (index_max > SYMB_LEN/2)
        *ptr_sampling_correction = 1;
    }
  
  /* ################################################################### */
  /* ############### the following is for debugging only ############### */
  /* ################################################################### */
  
#ifdef DEBUG_OUTPUT
  for (lag=0; lag<SYMB_LEN; lag++)
    {
      dbl_value = (double)(abs(bits_out[0]) & 0x0001);
      // dbl_value = (double)(demod_state->xcorr_t0[lag]);
      // dbl_value = (double)(xcorr_lp_t0[lag]);
      // dbl_value = (double)(diff[lag]);
      // dbl_value = (double)(xcorr_lp_t2[lag]);
      // dbl_value = (double)(demod_state->diff_smooth[lag]);
      // dbl_value = (double)(*ptr_sampling_correction);
      
      if (fwrite(&dbl_value, sizeof(double),1,out_file1) == 0)
        {
          fprintf(stderr, "Error while writing to file\n\n") ;
          exit(1);
        }
      dbl_value = (double)(soft_value);
      // dbl_value = (double)(xcorr_lp_t3[lag]);
      // dbl_value = (double)(demod_state->diff_smooth[lag]);
      // dbl_value = (double)(32768.0*soft_value);
      // dbl_value = (double)(diff[lag]);
      // dbl_value = (double)index_max;

      if (fwrite(&dbl_value, sizeof(double),1,out_file2) == 0)
        {
          fprintf(stderr, "Error while writing to file\n\n") ;
          exit(1);
        }
    } 
#endif
  
}


