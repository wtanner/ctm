/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : baudot_functions.c
*      Author           : EEDN/RV Matthias Doerbecker
*      Tested Platforms : Sun Solaris, MS Windows NT 4.0
*      Description      : Functions for Baudot Modulator and Demodulator
*                         (Fixed Point Version)
*
*      Changes since October 13, 2000:
*      - added reset functions 
*        reset_baudot_tonemod() and reset_baudot_tonedemod()
*
*      $Log: $
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/
#include "baudot_functions.h"
const char baudot_functions_id[] = "@(#)$Id: $" baudot_functions_h;

#include <stdio.h>

/*
*******************************************************************************
*                         LOCAL DEFINES
*******************************************************************************
*/

// #define DEBUG_OUTPUT  /* comment this out for debuging purposes */

#ifndef min
#define min(A,B) ((A) < (B) ? (A) : (B))
#endif
 
/* definitions for demodulator only */
#define OFFSET_NORMALISATION   60    /* ignore low-power audio samples  */
#define THRESHOLD_DIFF       2300    /* 0.07*32767 reliability threshold*/
#define THRESHOLD_STARTBIT      8    /* threshold for StartBit detection     */
#define DURATION_STARTDETECT   70    /* time interval for start bit detector */

/* definitions for modulator only */
#define NUM_STOP_BITS_TX        2    /* number of stop bits per character    */

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/



/****************************************************************************/
/* convertChar2ttyCode()                                                    */
/* *********************                                                    */
/* Conversion from character into tty code.                                 */
/*                                                                          */
/* TTY code is similar to Baudot Code, with the exception that bit5 is used */
/* for signalling whether the actual character is out of the Letters or the */
/* Figures character set. The remaining bits (bit0...bit4) are the same     */
/* than in Baudot Code.                                                     */
/*                                                                          */
/* input variables:                                                         */
/* - inChar       character that shall be converted                         */
/*                                                                          */
/* return value:  Baudot Code (0..63) of the input character                */
/*                or -1 in case that inChar is not valid (e.g. inChar=='\0')*/
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/02/17 */
/****************************************************************************/

Shortint convertChar2ttyCode(char inChar)
{
  const char ttyCharTab[] = "\bE\nA SIU\rDRJNFCKTZLWHYPQOBG\0MXV\0\b3\n- \087\r$4',!:(5\")2=6019\?+\0./;\0";
  
  Shortint  ttyCharCode=-1; 
  if (inChar != '\0')
    {
      /* determine the character's TTY code index */
      ttyCharCode=0; 
      while((inChar!=ttyCharTab[ttyCharCode]) && (ttyCharCode<64))
        ttyCharCode++;
      
    }
  if (ttyCharCode==64)
    ttyCharCode = -1;

  return ttyCharCode;
}



/****************************************************************************/
/* convertTTYcode2char()                                                    */
/* *********************                                                    */
/* Conversion from tty code into character                                  */
/*                                                                          */
/* input variables:                                                         */
/* - ttyCode      Baudot code (must be within the range 0...63)             */
/*                or -1 if there is nothing to convert                      */
/*                                                                          */
/* return value:  character (or '\0' if ttyCode is not valid)               */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/02/17 */
/****************************************************************************/

char convertTTYcode2char(Shortint ttyCode)
{
  const char ttyCharTab[] = "\bE\nA SIU\rDRJNFCKTZLWHYPQOBG\0MXV\0\b3\n- \087\r$4',!:(5\")2=6019\?+\0./;\0";
  char outChar = '\0';
  
  if ((ttyCode>=0) && (ttyCode<64))
    outChar = ttyCharTab[ttyCode];
  
  return outChar;
}



/****************************************************************************/
/* iir_filt()                                                               */
/* **********                                                               */
/* Recursive (IIR, infinte impulse response) digital filter according to    */
/* the following difference equation:                                       */
/*                                                                          */
/* y(n) = b(0)*x(n) + b(1)*x(n-1) + ... + b(filtOrder)*x(n-filtOrder)       */
/*                  - a(1)*y(n-1) - ... - a(filtOrder)*y(n-filtOrder)       */
/*                                                                          */
/* Note, that it is assumed that a(0)=0.                                    */
/*                                                                          */
/* input variables:                                                         */
/* - bufferIn   Vector with input samples [x(n), x(n-1),...x(n-filtOrder)]. */
/*              This vector must be updated externally, i.e. before         */
/*              calling iir_filt(), bufferIn has to be shifted to the       */
/*              right and bufferIn[0] must be assigned to the value of      */
/*              the actual input sample.                                    */
/* - aCoeff     Vector with coefficients [   1, a(1), a(2),... a(filtOrder) */
/* - bCoeff     Vector with coefficients [b(0), b(1), b(2),... b(filtOrder) */
/* - filtOrder  Order of the filter                                         */
/*                                                                          */
/* input/output variables:                                                  */
/* - bufferOut  Vector with output samples [y(n), y(n-1),...x(y-filtOrder)].*/
/*              This vector is updated internally by iir_filt(), i.e. no    */
/*              shift operations have to be performed externally. The       */
/*              output sample can be obtained from bufferOut[0].            */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/02/17 */
/****************************************************************************/

void iir_filt(Shortint* bufferOut, Shortint* bufferIn, 
              Shortint* aCoeff,    Shortint* bCoeff, 
              Shortint  filtOrder)
{
  Shortint cnt;
  Longint  sum;
  
  /* Update the shift register for output */
  for (cnt=filtOrder; cnt>0; cnt--)
    bufferOut[cnt] = bufferOut[cnt-1];
  
  sum = (Longint)(bufferIn[0])*(Longint)(bCoeff[0]);
  for (cnt=1; cnt<=filtOrder; cnt++)
    sum += ((Longint)(bufferIn[cnt])*(Longint)(bCoeff[cnt]) - 
            (Longint)(bufferOut[cnt])*(Longint)(aCoeff[cnt]));
  
  bufferOut[0] = (Shortint)(sum>>15);
}



/****************************************************************************/
/* init_baudot_tonedemod()                                                  */
/* ***********************                                                  */
/* Initialization of the demodulator for Baudot Tones.                      */
/*                                                                          */
/* input/output variables:                                                  */
/* - state        Pointer to the initialized state variable (must be        */
/*                allocated before calling init_baudot_tonedemod())         */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/02/17 */
/****************************************************************************/

void init_baudot_tonedemod(baudot_tonedemod_state_t* state)
{
  Shortint cnt;
  
  for (cnt=0; cnt<=BAUDOT_BIT_DURATION; cnt++)
    state->bufferDiff[cnt] = 0;

  for (cnt=0; cnt<=BAUDOT_BP_FILTERORDER; cnt++)
    {
      state->bufferToneVec[cnt] = 0;
      state->bufferAbsToneVec[cnt] = 0;
      state->bufferBP0out[cnt] = 0;
      state->bufferBP1out[cnt] = 0;
      state->bufferBP2out[cnt] = 0;
    }
  
  for (cnt=0; cnt<=BAUDOT_LP_FILTERORDER; cnt++)
    {
      state->bufferLP0out[cnt] = 0;
      state->bufferLP1out[cnt] = 0;
      state->bufferLP2out[cnt] = 0;
      state->bufferLP0in[cnt] = 0;
      state->bufferLP1in[cnt] = 0;
      state->bufferLP2in[cnt] = 0;
    }
  state->cntSamplesForStartBit= 0;
  state->cntSamplesForNextBit = 0;
  state->startBitDetected     = false;
  state->cntBitsActualChar    = 0;
  state->inFigureMode         = false;
}

/****************************************************************************/
/* reset_baudot_tonedemod()                                                 */
/****************************************************************************/

void reset_baudot_tonedemod(baudot_tonedemod_state_t* state)
{
  state->cntSamplesForStartBit= 0;
  state->cntSamplesForNextBit = 0;
  state->startBitDetected     = false;
  state->cntBitsActualChar    = 0;
  state->inFigureMode         = false;
}


/****************************************************************************/
/* baudot_tonedemod()                                                       */
/* ******************                                                       */
/* Demodulator for Baudot Tones.                                            */
/*                                                                          */
/* input variables:                                                         */
/* - toneVec           Vector containing the input audio signal             */
/* - numSamples        Length of toneVec                                    */
/*                                                                          */
/* input/output variables:                                                  */
/* - ptrOutFifoState   Pointer to the state of the output shift register    */
/*                     containing the demodulated extended TTY codes        */
/* - state             Pointer to the state variable of baudot_tonedemod()  */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/02/17 */
/****************************************************************************/

void baudot_tonedemod(Shortint* toneVec, Shortint numSamples,
                      fifo_state_t* ptrOutFifoState,
                      baudot_tonedemod_state_t* state)
{
#if BAUDOT_LP_FILTERORDER==1
  /* Coefficients for 1st order lowpass and 2nd order bandpass filters       */
  /* The corresponding floating point values are as follows:                 */
  /* aCoeffLowpass[]= {1.00000000000000,  -0.93906250581749};                */
  /* bCoeffLowpass[]= {0.03046874709125,   0.03046874709125};                */
  /* aCoeffBP1400[] = {1.00000000000000,-0.85592593938989, 0.88161859236319};*/
  /* bCoeffBP1400[] = {0.05919070381841,  0.0, -0.05919070381841};           */
  /* aCoeffBP1800[] = {1.00000000000000, -0.29493197879544,0.88161859236319};*/
  /* bCoeffBP1800[] = {0.05919070381841,  0.0, -0.05919070381841};           */
  
  static Shortint aCoeffLowpass[] = {32767, -30770};
  static Shortint bCoeffLowpass[] = {  998,    998};
  
  static Shortint aCoeffBP1400[]  = {32767, -28046, 28888};
  static Shortint bCoeffBP1400[]  = { 1940,      0, -1940};
  
  static Shortint aCoeffBP1800[]  = {32767,  -9664,  28888};
  static Shortint bCoeffBP1800[]  = { 1940,      0, -1940};
  
#endif

  Shortint  cnt;
  Shortint  cntSample;
  Shortint  diff;
  
#ifdef DEBUG_OUTPUT
  static FILE  *baudot_info_file;
  static Bool  firsttime=true;
  if (firsttime)
    {
      firsttime=false;
      if ((baudot_info_file=fopen("baudot_info.srt", "wb"))==NULL)
        {
          fprintf(stderr,"Error while opening baudot_info.srt\n\n") ;
          exit(1);
        }
    }
#endif
  
  /* The Baudot Detector is based on an observation of the signal diff,    */
  /* which represents the normalized difference of the envelopes in the    */
  /* 1400Hz band and in the 1800Hz band, respectively. The signal diff is  */
  /* obtained by signal processing according to the following scheme:      */
  /*                                                                       */
  /* audio in            +-------+   +----+   +---+                        */
  /* ------o------------>|BP 1400|-->| LP |-->| + |                        */
  /*       |             +-------+   +----+   |   |   +-----------+  diff  */
  /*       |                                  |   |-->| Normalize |------> */
  /*       |             +-------+   +----+   |   |   +-----------+        */
  /*       o------------>|BP 1800|-->| LP |-->| - |         ^              */
  /*       |             +-------+   +----+   +---+         |              */
  /*       |                                                |              */
  /*       |   +-----+   +-------+   +----+                 |              */
  /*       +-->| abs |-->|  LP   |-->| LP |-----------------+              */
  /*           +-----+   +-------+   +----+                                */
  
  for (cntSample=0; cntSample<numSamples; cntSample++)
    {
      for (cnt=BAUDOT_BP_FILTERORDER; cnt>0; cnt--)
        {
          state->bufferToneVec[cnt]    = state->bufferToneVec[cnt-1];
          state->bufferAbsToneVec[cnt] = state->bufferAbsToneVec[cnt-1];
        }
      state->bufferToneVec[0]    = (toneVec[cntSample])>>1;
      state->bufferAbsToneVec[0] = abs(toneVec[cntSample]>>1);
      
      iir_filt(state->bufferBP1out, state->bufferToneVec,
               aCoeffBP1400, bCoeffBP1400, BAUDOT_BP_FILTERORDER);
      iir_filt(state->bufferBP2out, state->bufferToneVec,
               aCoeffBP1800, bCoeffBP1800, BAUDOT_BP_FILTERORDER);
      iir_filt(state->bufferBP0out, state->bufferAbsToneVec,
               aCoeffLowpass, bCoeffLowpass, BAUDOT_LP_FILTERORDER);
      
      /* The filter "BP0" isn't really a bandpass. However, since it is */
      /* co-located in parallel to the bandpass filters BP1 and BP2, I  */
      /* have decided to use the name BP0. BP0 is rather a lowpass      */
      /* filter, which acts on the rectified input signal. The goal of  */
      /* this filter is to have a signal that represents the envelope   */
      /* of the input signal. This lowpass filter is designed such that */
      /* its impulse response is equal to the envelope of the           */
      /* bandpass filters BP1 and BP2.                                  */
      
      for (cnt=BAUDOT_LP_FILTERORDER; cnt>0; cnt--)
        {
          state->bufferLP0in[cnt] = state->bufferLP0in[cnt-1];
          state->bufferLP1in[cnt] = state->bufferLP1in[cnt-1];
          state->bufferLP2in[cnt] = state->bufferLP2in[cnt-1];
        }
      
      state->bufferLP0in[0] = abs(state->bufferBP0out[0]);
      state->bufferLP1in[0] = abs(state->bufferBP1out[0]);
      state->bufferLP2in[0] = abs(state->bufferBP2out[0]);

      iir_filt(state->bufferLP0out, state->bufferLP0in,
               aCoeffLowpass, bCoeffLowpass, BAUDOT_LP_FILTERORDER);
      iir_filt(state->bufferLP1out, state->bufferLP1in,
               aCoeffLowpass, bCoeffLowpass, BAUDOT_LP_FILTERORDER);
      iir_filt(state->bufferLP2out, state->bufferLP2in,
               aCoeffLowpass, bCoeffLowpass, BAUDOT_LP_FILTERORDER);
      
      /* diff is positive, if the power in the 1400 Hz band is higher than */
      /* the power in the 1800 Hz band. diff is negative if the power in   */
      /* the 1800 Hz band is higher. The magnnitude of diff provides       */
      /* reliability information, i.e. it indicates which amount of the    */
      /* input signal power is concentrated in the two band pass channels. */
      diff = ((((Longint)(state->bufferLP1out[0]) - 
                (Longint)(state->bufferLP2out[0]))<<14) /
              ((Longint)(state->bufferLP0out[0])+OFFSET_NORMALISATION));
      // fprintf(stderr, "%d, ", diff);
            
#ifdef DEBUG_OUTPUT
      if (fwrite(&diff, sizeof(Shortint),1, baudot_info_file) == 0)
        {
          fprintf(stderr,"Error while writing to baudot_info.srt\n\n");
          exit(1);
        }
#endif

      for (cnt=BAUDOT_BIT_DURATION; cnt>0; cnt--)
        state->bufferDiff[cnt] = state->bufferDiff[cnt-1];
      state->bufferDiff[0] = diff;
      
      if (!state->startBitDetected)
        {
          /* Start bit has not been detected yet: Since the start bit  */
          /* is always 0 (1800 Hz), we can detect the start bit by     */
          /* counting the number of samples for which diff is smaller  */
          /* (more negative) than                                      */
          /* THRESHOLD_STARTBIT*diff(n-bitDuration),                   */
          /* i.e. diff is compared to its (scaled) value one bit ago   */
          if ((Longint)diff < THRESHOLD_STARTBIT*
              (Longint)(min(state->bufferDiff[BAUDOT_BIT_DURATION], -328)))
            state->cntSamplesForStartBit++;
          else
            state->cntSamplesForStartBit=0;
          
          if ((state->cntSamplesForStartBit>=DURATION_STARTDETECT) &
              (abs(diff) > THRESHOLD_DIFF))
            {
              /* detectStartBit has exceeded its threshold for more than */
              /* DURATION_STARTDETECT samples and magnitude of diff is   */
              /* reliable enough --> This must be a start bit!!!         */ 
              state->startBitDetected=true;
              
              /* Reset the counter for the received bits of the actual   */
              /* character as well as the sample counter between         */
              /* adjacent bits.                                          */
              state->cntBitsActualChar = 0;
              state->cntSamplesForNextBit=0;
              state->ttyCode = 0;
            }
        }
      else
        {
          /* Start bit has already been detected                         */
          /* --> update the sample counter between adjacent bits         */
          state->cntSamplesForNextBit++;
          
          if(state->cntSamplesForNextBit>=BAUDOT_BIT_DURATION)
            {
              /* The time interval between the last bit and the next     */
              /* bit is over now --> check whether diff is reliable.     */
              if (abs(diff) <= THRESHOLD_DIFF)
                {
                  /* diff is not reliable enough -> discard all bits of  */
                  /* this character and wait for next start bit.         */
                  state->startBitDetected  = false;
                  state->cntBitsActualChar = 0;
                }
              else
                {
                  /* Check, whether the actual bit is still an info bit */
                  if(state->cntBitsActualChar < BAUDOT_NUM_INFO_BITS)
                    {
                      /* Receive and store the bit */
                      if (diff>0)
                        state->ttyCode =
                          state->ttyCode + (1<<(state->cntBitsActualChar));
                      
                      state->cntBitsActualChar++;
                      state->cntSamplesForNextBit=0;
                    }
                  else /* The actual bit is a stop bit */
                    {
                      if (diff<0)
                        /* The stop bit is not +1 (1400 Hz)          */
                        /* --> forget this character and do nothing! */
                        diff=diff;
                      else if(state->ttyCode==BAUDOT_SHIFT_FIGURES)
                        state->inFigureMode=true;
                      else if(state->ttyCode==BAUDOT_SHIFT_LETTERS)
                        state->inFigureMode=false;
                      else
                        {
                          if(state->inFigureMode)
                            state->ttyCode=state->ttyCode+32;
                          Shortint_fifo_push(ptrOutFifoState, 
                                             &(state->ttyCode), 1);
                        }
                      /* Now we have to wait again for the next start bit */
                      state->startBitDetected  = false;
                      state->cntBitsActualChar = 0;
                    }
                }
            }
        }
    }
}


/****************************************************************************/
/* init_baudot_tonemod()                                                    */
/* *********************                                                    */
/* Initialization of the modulator for Baudot Tones.                        */
/*                                                                          */
/* input/output variables:                                                  */
/* - state        Pointer to the initialized state variable (must be        */
/*                allocated before calling init_baudot_tonedemod())         */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/02/17 */
/****************************************************************************/

void init_baudot_tonemod(baudot_tonemod_state_t* state)
{
  state->phaseValue             = 0;
  state->cntSample              = 0;
  state->cntCharsSinceLastShift = 72;   /* this generates an initial SHIFT */
  state->inFigureMode           = false;
  state->txBitAvailable         = false;
  state->tailBitsGenerated      = true;
  
  Shortint_fifo_init(&(state->fifo_state), 32);
}

/****************************************************************************/
/* reset_baudot_tonemod()                                                   */
/****************************************************************************/

void reset_baudot_tonemod(baudot_tonemod_state_t* state)
{
  state->phaseValue             = 0;
  state->cntSample              = 0;
  state->cntCharsSinceLastShift = 72;   /* this generates an initial SHIFT */
  state->inFigureMode           = false;
  state->txBitAvailable         = false;
  state->tailBitsGenerated      = true;
  
  Shortint_fifo_reset(&(state->fifo_state));
}

/****************************************************************************/
/* baudot_tonemod()                                                         */
/* ****************                                                         */
/* Modulator for Baudot Tones.                                              */
/*                                                                          */
/* input variables:                                                         */
/* - inputTTYcode      TTY code of the character that has to be modulated.  */
/*                     inputTTYcode must be in the range 0...63, otherwise  */
/*                     it is assumed that there is no character to modulate.*/
/* - lengthToneVec     Indicates how many samples have to be generated.     */
/*                                                                          */
/* output variables:                                                        */
/* - outputToneVec             Vector where the output samples are written  */
/*                             to.                                          */
/* - ptrNumBitsStillToModulate Indicates how many bits are still in the     */
/*                             fifo buffer                                  */
/*                                                                          */
/* input/output variables:                                                  */
/* - state             Pointer to the state variable of baudot_tonedemod()  */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/02/17 */
/****************************************************************************/

void baudot_tonemod(Shortint  inputTTYcode,
                    Shortint *outputToneVec,
                    Shortint  lengthToneVec,
                    Shortint *ptrNumBitsStillToModulate,
                    baudot_tonemod_state_t* state)
{
  Shortint   cnt;
  Shortint   cntTxBits=0;
  
  const Shortint sinTable[] = 
  {0,        5126,  10126,  14876,  19260,  
   23170,   26509,  29196,  31163,  32364,  
   32767,   32364,  31163,  29196,  26509, 
   23170,   19260,  14876,  10126,   5126,
   -0,      -5126, -10126, -14876, -19260,  
   -23170, -26509, -29196, -31163, -32364,  
   -32767, -32364, -31163, -29196, -26509, 
   -23170, -19260, -14876, -10126,  -5126};
  
  /* The following vector is static in order to prevent a reallocation   */
  /* with each call of this function. The contents of the vector is not  */
  /* required after leaving this function, therefore the use of static   */
  /* variables does not prevent multiple instances of this function.     */
  
  static Shortint  TxBitsBuffer[2*(1+BAUDOT_NUM_INFO_BITS+NUM_STOP_BITS_TX)];
  
  /* Check, whether actual character is valid */
  if ((inputTTYcode>=0) && (inputTTYcode<64))
    {
      /* ShiftToLetters/SiftToFigures have to be generated, if the     */
      /* actual character and the current transmitter mode do not fit. */
      /* Additionally, an appropriate Shift symbol is sent at least    */
      /* once for each interval of 72 characters.                      */
      
	  if ((inputTTYcode>=32) && 
		  ((!(state->inFigureMode)) || (state->cntCharsSinceLastShift>=72)))
        {
          /* send BAUDOT_SHIFT_FIGURES */
          TxBitsBuffer[cntTxBits++] = 0; /* start bit */
          for (cnt=0; cnt<BAUDOT_NUM_INFO_BITS; cnt++)
            TxBitsBuffer[cntTxBits++] = ((BAUDOT_SHIFT_FIGURES >> cnt) & 1);
          for (cnt=0; cnt<NUM_STOP_BITS_TX; cnt++)
            TxBitsBuffer[cntTxBits++] = 1; /* stop bit */
          state->cntCharsSinceLastShift = 0;
          state->inFigureMode           = true;
        }
      
      if ((inputTTYcode<32) && 
		  ((state->inFigureMode) || (state->cntCharsSinceLastShift>=72)))
        {
          /* send BAUDOT_SHIFT_LETTERS */
          TxBitsBuffer[cntTxBits++] = 0; /* start bit */
          for (cnt=0; cnt<BAUDOT_NUM_INFO_BITS; cnt++)
            TxBitsBuffer[cntTxBits++] = ((BAUDOT_SHIFT_LETTERS >> cnt) & 1);
          for (cnt=0; cnt<NUM_STOP_BITS_TX; cnt++)
            TxBitsBuffer[cntTxBits++] = 1; /* stop bit */
          state->cntCharsSinceLastShift = 0;
          state->inFigureMode           = false;
        }
      
      /* send inputTTYcode */
      TxBitsBuffer[cntTxBits++] = 0; /* start bit */
      for (cnt=0; cnt<BAUDOT_NUM_INFO_BITS; cnt++)
        TxBitsBuffer[cntTxBits++] = ((inputTTYcode >> cnt) & 1);
      for (cnt=0; cnt<NUM_STOP_BITS_TX; cnt++)
        TxBitsBuffer[cntTxBits++] = 1; /* stop bit */
      (state->cntCharsSinceLastShift)++;
      
      /* push all TxBits into the fifo buffer */
      Shortint_fifo_push(&(state->fifo_state), TxBitsBuffer, cntTxBits);
      state->tailBitsGenerated = false;
    }
  else
    {
      if ((Shortint_fifo_check(&(state->fifo_state))<=1) && 
          !(state->tailBitsGenerated))
        {
          for (cnt=0; cnt<8; cnt++)
            TxBitsBuffer[cnt] = 1;
          Shortint_fifo_push(&(state->fifo_state), TxBitsBuffer, 8);
          state->tailBitsGenerated = true;
        }
    }
  
  /* Now the output samples are generated */
  
  for (cnt=0; cnt<lengthToneVec; cnt++)
    {
      if (state->cntSample == 0)
        {
          /* the last bit has been modulated completely, therefore */
          /* a new bit has to be popped from the fifo buffer       */
          if (Shortint_fifo_check(&(state->fifo_state))>0)
            {
              Shortint_fifo_pop(&(state->fifo_state), &(state->txBitActual),1);
              state->txBitAvailable = true;
            }
          else
            state->txBitAvailable = false;
        }
      
      /* Do the modulation or generate zero output */
      /* if there is no bit available              */
      
      if (state->txBitAvailable)
        {
          /* phaseValue corresponds to the mathematical phase as follows: */
          /* phase = 2*pi*phaseValue*200/8000                             */
          state->phaseValue = state->phaseValue + 9-2*state->txBitActual;
          
          /* check whether phase is > 2*pi */
          if (state->phaseValue >= 40)
            state->phaseValue = state->phaseValue-40;
          
          outputToneVec[cnt] = (sinTable[state->phaseValue])>>1;
          
          state->cntSample++;
          if (state->cntSample >= BAUDOT_BIT_DURATION)
            state->cntSample = 0;
        }
      else
        {
          state->phaseValue  = 0;
          outputToneVec[cnt] = 0;
        }
    }
  
  /* Determine, how many bits still have to be modulated (consider also */
  /* the bit which is actually beeing modulated).                       */
  *ptrNumBitsStillToModulate = Shortint_fifo_check(&(state->fifo_state));
  if (state->cntSample > 0)
    (*ptrNumBitsStillToModulate)++;
}
