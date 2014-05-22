
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <unistd.h>

#ifdef __OpenBSD__
#include <sys/types.h>
#include <sndio.h>
#endif

#include <ctype.h>
#include <termios.h>

#include "ctm.h"
#include "ctm_defines.h"
#include "ctm_transmitter.h"
#include "ctm_receiver.h"
#include "baudot_functions.h"
#include "ucs_functions.h"
#include <typedefs.h>
#include <fifo.h>

static Shortint ttyCode;
static char character;
static UShortint ucsCode;
static Shortint cnt;

/* function prototypes */
static void layer2_process_ctm_in(struct ctm_state *);
static void layer2_process_ctm_out(struct ctm_state *);
void layer2_process_user_input(struct ctm_state *);
void layer2_process_user_output(struct ctm_state *);
void layer2_process_ctm_audio_in(struct ctm_state *);
void layer2_process_ctm_audio_in(struct ctm_state *);
void layer2_process_ctm_audio_out(struct ctm_state *);
void layer2_process_ctm_file_input(struct ctm_state *);
void layer2_process_ctm_file_output(struct ctm_state *);
static void layer2_process_ctm_out(struct ctm_state *);


void layer2_process_user_input(struct ctm_state *state)
{
  if (state->baudotReadFromFile)
  {
    /* if the baudot out FIFO isn't already full, grab more samples. */
    if (Shortint_fifo_check(&(state->baudotOutTTYCodeFifoState)) < state->baudotOutTTYCodeFifoLength) {
      if(read(state->userInputFileFp, state->baudot_input_buffer, state->audio_buffer_size) < state->audio_buffer_size)
      {
        /* if EOF is reached, use buffer with zeros instead */
        state->baudotEOF = true;
        for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
          state->baudot_input_buffer[cnt]=0;
      }

#ifdef LSBFIRST
      if (state->compat_mode)
      {
        /* The test pattern baudot PCM files are in big-endian. If we are on a little-endian machine, we will need to swap the bytes */
        for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
        {
          state->baudot_input_buffer[cnt] = swap16(state->baudot_input_buffer[cnt]);
        }
      }
#endif
    }

    /* Run the Baudot demodulator */
    baudot_tonedemod(state->baudot_input_buffer, LENGTH_TONE_VEC, 
        &(state->baudotOutTTYCodeFifoState), &(state->baudot_tonedemod_state));
    /* Adjust the Mode of the modulator according to the demodulator */
    state->baudot_tonemod_state.inFigureMode = state->baudot_tonedemod_state.inFigureMode;

    /* Set flag indicating that the demodulator has already */
    /* decoded a Baudot character.                          */ 
    if(Shortint_fifo_check(&(state->baudotOutTTYCodeFifoState))>0)
      state->baudotAlreadyReceived = true;

    /* Determine wheter the demodulator has detected that a Baudot */
    /* character is actually received. This decision must be made  */
    /* before the complete character has been received so that the */
    /* original audio signal can be muted before a successive      */
    /* Baudot detector is able to decode this character. We        */
    /* make this decision after receiving the start bit and the    */
    /* five information bits completely. However, for the          */
    /* first character, we postulate additionally, that at least   */
    /* one information bit is +1 (1400 Hz). Since the start bit    */
    /* is always zero, this decision rule requires a transition    */
    /* from 1800 Hz to 1400 Hz, which reduces the danger of false  */
    /* alarms for pure voice calls. Since every Baudot             */
    /* transmission shall start with a SHIFT symbol, this          */
    /* assumption (at least one bit has to be +1) is fulfilled     */
    /* for every Baudot transmission.                              */

    //fprintf(stderr, "%d,", baudot_tonedemod_state.cntBitsActualChar);

    if (state->baudot_tonedemod_state.cntBitsActualChar>=5)
    {
      if (state->baudotAlreadyReceived)
        state->actualBaudotCharDetected = true;
      else
        state->actualBaudotCharDetected = (state->baudot_tonedemod_state.ttyCode>0);
    }
    else
      state->actualBaudotCharDetected = false;

    /* The next lines guarantee that the Baudot signal is muted even */
    /* if the received character is a SHIFT symbol (a SHIFT symbol   */
    /* would not set the CTM transmitter into an active state).      */ 
    if (state->cntHangoverFramesForMuteBaudot>0) 
      state->cntHangoverFramesForMuteBaudot--;
    if (state->actualBaudotCharDetected)
      state->cntHangoverFramesForMuteBaudot = 1+(320/LENGTH_TONE_VEC);
  }

  else {
    /* otherwise we are reading text input */
    if (Shortint_fifo_check(&(state->baudotOutTTYCodeFifoState)) < state->baudotOutTTYCodeFifoLength)
    {
      if (read(state->userInputFileFp, &character, 1) < 1)
      {
        /* reuse baudot EOF flag to tell the program no more input */
        state->baudotEOF = true;
      }
      else {
        ttyCode = convertChar2ttyCode(character);
        Shortint_fifo_push(&(state->baudotOutTTYCodeFifoState), &ttyCode, 1);
      }
    }
  }
} 

void layer2_process_user_output(struct ctm_state *state)
{
  /* If there are characters from the CTM receiver, or if the CTM     */
  /* receiver has detected a synchronisation preamble, or if the      */
  /* Baudot Modulator is still busy (i.e. there are still bits to     */
  /* modulate) --> run the Baudot Modulator (again). This branch is   */
  /* also executed (but with the Baudot Modulator in idle mode), if   */
  /* the ctm_receiver has indicated that early_muting is required.    */
  if ((Shortint_fifo_check(&(state->ctmToBaudotFifoState)) >0) || 
      (state->numBaudotBitsStillToModulate>0) || state->earlyMutingRequired)
  {
    /* If there is a character on the ctmToBaudotFifo available and */
    /* if the Baudot modulator is able to process a new character   */
    /* --> pop the character from the fifo.                         */
    if ((Shortint_fifo_check(&(state->ctmToBaudotFifoState)) >0) &&
        (((state->numBaudotBitsStillToModulate <= 8) &&
        (state->cntFramesSinceLastBypassFromCTM>=10*160/LENGTH_TONE_VEC)) || state->writeToTextFile))
      Shortint_fifo_pop(&state->ctmToBaudotFifoState, &ttyCode, 1);
    else
      ttyCode = -1;

    if (state->baudotWriteToFile) {
      baudot_tonemod(ttyCode, state->baudot_output_buffer, LENGTH_TONE_VEC,
          &(state->numBaudotBitsStillToModulate),
          &(state->baudot_tonemod_state));
      /* Adjust the Mode of the demodulator according to the modulator */
      state->baudot_tonedemod_state.inFigureMode 
        = state->baudot_tonemod_state.inFigureMode;

      if (state->cntFramesSinceLastBypassFromCTM<maxShortint)
        state->cntFramesSinceLastBypassFromCTM++;
    }
    else if (ttyCode != - 1) {
      character = convertTTYcode2char(ttyCode);
      if (write(state->userOutputFileFp, &character, 1) == -1)
        errx(1, "error writing to text output file, file descriptor %d.", state->userOutputFileFp);
    }
  }
  else
  {
    /* If the modulator has not been run                   */
    /* --> copy samples from the input to the output       */
    /*     or set them to zero, if bypassing is prohibited */
    if (!state->disableBypass)
      for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
        state->baudot_output_buffer[cnt] = state->ctm_input_buffer[cnt];
    else
      for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
        state->baudot_output_buffer[cnt] = 0;
    state->cntFramesSinceLastBypassFromCTM = 0;
  }

  /* decide which user output we are and write it. */
  if(state->baudotWriteToFile) {
#ifdef LSBFIRST
    if (state->compat_mode)
    {
      /* The test pattern baudot PCM files are in big-endian. If we are on a little-endian machine, we will need to swap the bytes */
      for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
      {
        state->baudot_output_buffer[cnt] = swap16(state->baudot_output_buffer[cnt]);
      }
    }
#endif
    if (write(state->userOutputFileFp, state->baudot_output_buffer, state->audio_buffer_size) == -1)
      errx(1, "error writing to baudot output file.");
  }
}

void layer2_process_ctm_audio_in(struct ctm_state *state)
{
  if (sio_read(state->audio_hdl, state->ctm_input_buffer, state->audio_buffer_size) < state->audio_buffer_size) {
    warnx("underrun in audio input from device.");
  }

  layer2_process_ctm_in(state);
}

void layer2_process_ctm_audio_out(struct ctm_state *state)
{
  layer2_process_ctm_out(state);

  if (sio_write(state->audio_hdl, state->ctm_output_buffer, state->audio_buffer_size) < state->audio_buffer_size) {
    warnx("overrun in audio output to device.");
  }
}

void layer2_process_ctm_file_input(struct ctm_state *state)
{
  if(read(state->ctmInputFileFp, state->ctm_input_buffer, state->audio_buffer_size) < state->audio_buffer_size) 
  {
    /* if EOF is reached, use buffer with zeros instead */
    state->ctmEOF = true;
    for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
      state->ctm_input_buffer[cnt]=0;
  }

#ifdef LSBFIRST
  if (state->compat_mode)
  {
    /* The test pattern baudot PCM files are in big-endian. If we are on a little-endian machine, we will need to swap the bytes */
    for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
    {
      state->ctm_input_buffer[cnt] = swap16(state->ctm_input_buffer[cnt]);
    }
  }
#endif

  layer2_process_ctm_in(state);
}

void layer2_process_ctm_file_output(struct ctm_state *state)
{
  layer2_process_ctm_out(state);

#ifdef LSBFIRST
  /* The test pattern baudot PCM files are in big-endian. If we are on a little-endian machine, we will need to swap the bytes */
  if (state->compat_mode)
  {
    for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
    {
      state->ctm_output_buffer[cnt] = swap16(state->ctm_output_buffer[cnt]);
    }
  }
#endif

  if (write(state->ctmOutputFileFp, state->ctm_output_buffer, state->audio_buffer_size) < state->audio_buffer_size)
    errx(1, "layer2_process_ctm_file_output: write error.");
}

static void layer2_process_ctm_in(struct ctm_state *state)
{
  /* Run the CTM receiver */

  Shortint_fifo_push(&(state->signalFifoState), state->ctm_input_buffer, 
      LENGTH_TONE_VEC);

  ctm_receiver(&(state->signalFifoState), &(state->ctmOutTTYCodeFifoState), &(state->earlyMutingRequired), &(state->rx_state));

  state->enquiryFromFarEndDetected = false;

  /* Check whether the far-end side is able to support CTM signals */
  if ((state->rx_state.wait_state.sync_found) && (!state->ctmFromFarEndDetected))
  {
    state->ctmFromFarEndDetected = true;
    fprintf(stderr, ">>> CTM from far-end detected! <<<\n");

    /* If we have not transmitted CTM tones so far, we should */
    /* treat the received burst as an enquiry burst.          */
    if (!state->ctmCharacterTransmitted)
    {
      fprintf(stderr,">>> Enquiry From Far End Detected! <<<\n");
      state->enquiryFromFarEndDetected=true;
      state->cntFramesSinceEnquiryDetected=0;
    }
  }

  /* If there is a character from the CTM receiver available          */
  /* --> print it on the screen and push it into the correct fifo.    */
  if (Shortint_fifo_check(&(state->ctmOutTTYCodeFifoState)) >0)
  {
    Shortint_fifo_pop(&(state->ctmOutTTYCodeFifoState), &ucsCode, 1);

    /* Check whether this was an enquiry burst from the other */
    /* side. Ignore this enquiry, if the last enquiry has     */
    /* been detected less than 25 frames (500 ms) ago.        */
    if ((ucsCode==ENQU_SYMB) && 
        (state->cntFramesSinceEnquiryDetected > 25*160/LENGTH_TONE_VEC))
    {
      state->enquiryFromFarEndDetected=true;
      state->cntFramesSinceEnquiryDetected=0;
    }
    else
    {
      /* Convert character from UCS to Baudot code, print   */
      /* it on the screen and push it into ctmToBaudotFifo. */ 
      character = toupper(convertUCScode2char(ucsCode));
      ttyCode   = convertChar2ttyCode(character);
      if (ttyCode >= 0)
      {
        fprintf(stderr, "%c", character);
        Shortint_fifo_push(&(state->ctmToBaudotFifoState), &ttyCode, 1);
      }
    }
  }

  if (state->cntFramesSinceEnquiryDetected<maxShortint)
    state->cntFramesSinceEnquiryDetected++;
}

static void layer2_process_ctm_out(struct ctm_state *state)
{
  if (state->enquiryFromFarEndDetected)
  {
    /* Generate Acknowledgement burst, if Enquiry from the far side */
    /* has been detected. The code 0xFFFF has a special meaning,    */
    /* see description of function ctm_transmitter()                */
    if (Shortint_fifo_check(&(state->baudotToCtmFifoState))==0)
    {
      ucsCode = 0xFFFF;
      Shortint_fifo_push(&(state->baudotToCtmFifoState), &ucsCode, 1);
      state->ctmCharacterTransmitted   = true;
      state->enquiryFromFarEndDetected = false;
    }
  }
  else if (!state->ctmFromFarEndDetected && state->ctmTransmitterIsIdle &&
      (Shortint_fifo_check(&(state->baudotOutTTYCodeFifoState))>0) &&
      (state->cntTransmittedEnquiries<NUM_ENQUIRY_BURSTS))
  {
    /* Generate an Enquiry Burst, if one of the following            */
    /* conditions is fulfilled:                                      */
    /* - The first five bits of a character have been detected       */
    /*   by the Baudot demodulator and no CTM frames have been       */
    /*   transmitted so far.                                         */
    /* - There is at least one character to transmit, but CTM        */
    /*   receiver hasn't received an acknowledgement for the far-end */
    /*   so far. In this case an Enquiry Burst is initiated, if the  */
    /*   last Burst is finished and if the number of enquiry bursts  */
    /*   doesn't exceed NUM_ENQUIRY_BURSTS.                          */

    fprintf(stderr, ">>> Enquiry Burst generated! <<<\n");
    ucsCode = ENQU_SYMB;
    Shortint_fifo_push(&(state->baudotToCtmFifoState), &ucsCode, 1);
    state->ctmCharacterTransmitted = true;
    if (state->cntTransmittedEnquiries<maxShortint)
      state->cntTransmittedEnquiries++;
  }

  /* The CTM transmitter is executed in the following cases:            */
  /* - Character from Baudot demodulator available or start + info bits */
  /*   detected, provided that we know that the other side supports CTM */
  /*   or if we are still in the negotiation phase                      */
  /* - There is still a character in the baudotToCtmFifo                */
  /* - The CTM Modulator is still running                               */
  /* Otherwise, the audio samples are bypassed!                         */

  if((((Shortint_fifo_check(&(state->baudotOutTTYCodeFifoState))>0) || 
          (state->actualBaudotCharDetected || (state->cntHangoverFramesForMuteBaudot>0))) && 
        (state->ctmFromFarEndDetected || (state->cntFramesSinceBurstInit<ENQUIRY_TIMEOUT))) ||
      (Shortint_fifo_check(&(state->baudotToCtmFifoState))>0) || 
      (state->numCTMBitsStillToModulate>0) || state->tx_state.burstActive ||
      ((state->cntHangoverFramesForMuteBaudot>0)
       &&(state->cntTransmittedEnquiries<NUM_ENQUIRY_BURSTS)))
  {
    /* Transition from idle into active mode represents start of burst */
    if (state->ctmTransmitterIsIdle)
      state->cntFramesSinceBurstInit = 0;

    /* If there is a character from the Baudot receiver available     */
    /* and if the CTM transmitter is able to process a new character: */
    /* pop the character from the fifo.                               */

    if ((state->ctmFromFarEndDetected || (state->cntFramesSinceBurstInit<ENQUIRY_TIMEOUT)) &&
        (Shortint_fifo_check(&(state->baudotToCtmFifoState))==0))
    {
      if (Shortint_fifo_check(&(state->baudotOutTTYCodeFifoState))>0)
      {
        Shortint_fifo_pop(&(state->baudotOutTTYCodeFifoState), &ttyCode, 1);
        character = convertTTYcode2char(ttyCode);
        fprintf(stderr, "%c", character);
        ucsCode = convertChar2UCScode(character);
        Shortint_fifo_push(&(state->baudotToCtmFifoState), &ucsCode, 1);
      }
    }

    if ((Shortint_fifo_check(&(state->baudotToCtmFifoState))>0) &&
        (state->numCTMBitsStillToModulate<2*LENGTH_TX_BITS))
      Shortint_fifo_pop(&(state->baudotToCtmFifoState), &ucsCode, 1);
    else
      ucsCode = 0x0016;

    ctm_transmitter(ucsCode, state->ctm_output_buffer, &(state->tx_state), 
        &(state->numCTMBitsStillToModulate), state->sineOutput);

    state->ctmTransmitterIsIdle    
      = !state->tx_state.burstActive && (state->numCTMBitsStillToModulate==0);
    state->ctmCharacterTransmitted = true;
    state->syncOnBaudot = false;
  }
  else
  {
    /* After terminating a CTM burst, the forwarding of the original */
    /* audio signal might have to be delayed if there is actually    */
    /* a Baudot character in progress. In this case, we have to mute */
    /* the output signal until the character is completed.           */
    /* The audio signal is also muted if bypassing has been          */
    /* prohibited by the user (i.e. by using the option -nobypass)   */
    if ((!state->syncOnBaudot && 
          (state->baudot_tonedemod_state.cntBitsActualChar>0)) || state->disableBypass)
      for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
        state->ctm_output_buffer[cnt] = 0;
    else
    {
      /* Bypass audio samples. */
      for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
        state->ctm_output_buffer[cnt] = state->baudot_input_buffer[cnt];
      state->syncOnBaudot = true;
    }

    /* discard characters in oder to avoid FIFO buffer overflows */
    if (Shortint_fifo_check(&(state->baudotOutTTYCodeFifoState))>=
        state->baudotOutTTYCodeFifoLength-1)
      Shortint_fifo_pop(&(state->baudotOutTTYCodeFifoState), &ttyCode, 1);
  }

  if (state->cntFramesSinceBurstInit<maxShortint)
    state->cntFramesSinceBurstInit++;

  state->cntProcessedSamples += LENGTH_TONE_VEC;
}
