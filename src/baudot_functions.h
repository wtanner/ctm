/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : baudot_functions.h
*      Author           : EEDN/RV Matthias Doerbecker
*      Tested Platforms : Sun Solaris, Windows NT 4.0
*      Description      : header file for baudot_functions.c
*
*      Changes since October 13, 2000:
*      - added reset functions 
*        reset_baudot_tonemod() and reset_baudot_tonedemod()
*
*      $Log: $
*
*******************************************************************************
*/
#ifndef baudot_functions_h
#define baudot_functions_h "$Id: $"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include <typedefs.h>
#include <fifo.h>
#include <stdio.h>

/*
*******************************************************************************
*                         DEFINITIONS
*******************************************************************************
*/

/* definitions (might be of global interest) */
#define BAUDOT_NUM_INFO_BITS    5  /* number of info bits per character      */
#define BAUDOT_SHIFT_FIGURES   27  /* code of shift to figures symbol        */
#define BAUDOT_SHIFT_LETTERS   31  /* code of shift to letters symbol        */
#define BAUDOT_BIT_DURATION   176  /* must be 176/160 for 45.45/50.0 bit/s   */

/* more definitions (only of local interest, */
/* but required for the following typedefs)  */
#define BAUDOT_LP_FILTERORDER      1
#define BAUDOT_BP_FILTERORDER      2*BAUDOT_LP_FILTERORDER


/* ******************************************************************/
/* Type definitions for variables that contain all states of the    */
/* Baudot demodulator and modulator, respectively                   */
/* ******************************************************************/

typedef struct {
  Shortint    bufferToneVec[BAUDOT_BP_FILTERORDER+1];
  Shortint    bufferAbsToneVec[BAUDOT_BP_FILTERORDER+1];
  Shortint    bufferBP0out[BAUDOT_BP_FILTERORDER+1];
  Shortint    bufferBP1out[BAUDOT_BP_FILTERORDER+1];
  Shortint    bufferBP2out[BAUDOT_BP_FILTERORDER+1];
  Shortint    bufferLP0out[BAUDOT_LP_FILTERORDER+1];
  Shortint    bufferLP1out[BAUDOT_LP_FILTERORDER+1];
  Shortint    bufferLP2out[BAUDOT_LP_FILTERORDER+1];
  Shortint    bufferLP0in[BAUDOT_LP_FILTERORDER+1];
  Shortint    bufferLP1in[BAUDOT_LP_FILTERORDER+1];
  Shortint    bufferLP2in[BAUDOT_LP_FILTERORDER+1];
  Shortint    bufferDiff[BAUDOT_BIT_DURATION+1];
  Shortint    ttyCode;
  Shortint    cntBitsActualChar;
  Shortint    cntSamplesForStartBit;
  Shortint    cntSamplesForNextBit;
  Bool        startBitDetected;
  Bool        inFigureMode;
} baudot_tonedemod_state_t;



typedef struct {
  Shortint       phaseValue;
  Shortint       cntSample;
  Shortint       txBitActual;
  Shortint       cntCharsSinceLastShift;
  Bool           inFigureMode;
  Bool           txBitAvailable;
  Bool           tailBitsGenerated;
  fifo_state_t   fifo_state;
} baudot_tonemod_state_t;



/****************************************************************************/
/* convertChar2ttyCode()                                                    */
/* *********************                                                    */
/* Conversion from character into tty code                                  */
/*                                                                          */
/* input variables:                                                         */
/* - inChar       charcater that shall be converted                         */
/*                                                                          */
/* return value:  baudot code of the input character                        */
/*                or -1 in case that inChar is not valid (e.g. inChar=='\0')*/
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/02/17 */
/****************************************************************************/

Shortint convertChar2ttyCode(char inChar);



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

char convertTTYcode2char(Shortint ttyCode);



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

void init_baudot_tonedemod(baudot_tonedemod_state_t* state);

void reset_baudot_tonedemod(baudot_tonedemod_state_t* state);


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
/*                     containing the demodulated TTY codes                 */
/* - state             Pointer to the state variable of baudot_tonedemod()  */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/02/17 */
/****************************************************************************/

void baudot_tonedemod(Shortint* toneVec, Shortint numSamples,
                      fifo_state_t* ptrOutFifoState,
                      baudot_tonedemod_state_t* state);

void reset_baudot_tonemod(baudot_tonemod_state_t* state);


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

void init_baudot_tonemod(baudot_tonemod_state_t* state);



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
                    baudot_tonemod_state_t* state);
  
#endif
