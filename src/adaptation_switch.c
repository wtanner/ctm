/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : adaptation_switch.c
*      Purpose          : main function for the "Signal Adaptation Module"
*                         for conversion between Baudot Code and CTM signals
*                         with automatic switching between speech and
*                         modem signals
*
*      Changes since October 13, 2000:
*      - ShiftLtrs/ShiftFigs Symbols on the Baudot Code Leg of the adaptation
*        Module have influence on both transmssion directions now, i.e. the
*        Baudot Code Detector and the Regenerator are both in the same mode.
*      - Automatic Muting of the Baudot Code input signal as long as the 
*        Baudot Code regenerator is active. This is necessary in order to
*        avoid the forwarding of echoes that are caused on the PSTN side.
*      - Improved muting of the Baudot Code signal, even if the received 
*        character is a SHIFT symbol (a SHIFT symbol would not set the 
*        CTM transmitter into an active state)
*      - The complete program can now be executed on a sample-by-sample basis.
*        This is achieved by modifying the constant LENGTH_TONE_VEC, which
*        is defined in ctm_defines.h.  While default value of this constant
*        is 160, it has to be set to 1 if a sample-by-sample execution is 
*        desired. Note, that the timing between the block-based and the
*        sample-based is slightly different, so that the output signals are
*        not exactly the same.
*
*******************************************************************************
*
* $Id: $ 
*
*/

#include "ctm.h"
#include "ctm_defines.h"
#include "ctm_transmitter.h"
#include "ctm_receiver.h"
#include "baudot_functions.h"
#include "ucs_functions.h"
#include <typedefs.h>
#include <fifo.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#include <ctype.h>      /* toupper() */
#include <termios.h>

#ifdef __OpenBSD__
#include <sys/types.h>
#include <sndio.h>
#define PCAUDIO
#endif

/***********************************************************************/

void writeHead()
{
  fprintf(stderr, "\n");
  fprintf(stderr, "********************************************************************\n");
  fprintf(stderr, "  Cellular Text Telephone Modem (CTM) - Example Implementation for  \n");
  fprintf(stderr, "  Conversion between CTM and Baudot Code (use option -h for help)   \n");
  fprintf(stderr, "********************************************************************\n\n");
}

/***********************************************************************/

void usage()
{
  fprintf(stderr, "                     +------------+                 \n");
  fprintf(stderr, "  Baudot Tones  ---> |            | ---> CTM signal \n");
  fprintf(stderr, "                     | adaptation |                 \n");
  fprintf(stderr, "  Baudot Tones  <--- |            | <--- CTM signal \n");
  fprintf(stderr, "                     +------------+                 \n");
  fprintf(stderr, "use: ctm [ arguments ]\n");
  fprintf(stderr, "  -ctmin      <input_file>   input file with CTM signal \n");
  fprintf(stderr, "  -ctmout     <output_file>  output file for CTM signal \n");
  fprintf(stderr, "  -baudotin   <input_file>   input file with Baudot Tones \n");
  fprintf(stderr, "  -baudotout  <output_file>  output file for Baudot Tones \n");
  fprintf(stderr, "  -textout    <text_file>    output text file from CTM receiver (optional)\n");
  fprintf(stderr, "  -textin     <text_file>    input text file to CTM transmitter (optional)\n");
  fprintf(stderr, "  -numsamples <number>       number of samples to process (optional)\n");
  fprintf(stderr, "  -nonegotiation             disables the negotiation (optional)\n");
  fprintf(stderr, "  -nobypass                  disables the signal bypass (optional)\n");
  fprintf(stderr, "  -compat                     enables compatibility mode with 3GPP test files (optional)\n\n");
  exit(1);
}

int
open_file_or_stdio(const char *filename, int flags)
{
  int file_fd;
  if (!strncmp(filename, "-", 1))
  {
    if (flags & O_WRONLY)
      file_fd = STDOUT_FILENO;
    else if (flags & O_RDONLY)
      file_fd = STDIN_FILENO;
    else
        errx(1, "invalid read/write flag.");
    }

  else
  {
    if ((file_fd = open(filename, flags)) == -1)
      err(1, "%s", filename);
    }

  return file_fd;
}

/***********************************************************************/

int main(int argc, const char** argv)
{
  Shortint     cnt;
  int          optindex;

  /* command-line argument flags and variables */
  int ctm_input_fd;
  int ctm_output_fd;
  int user_input_fd;
  int user_output_fd;
  int compat_flag;
  int baudot_flag;
  int negotiation_flag;
  int ctm_file_mode_flag;
  int audio_mode_flag;
  ctm_user_input_mode user_input_mode;
  ctm_output_mode ctm_mode;
  
  writeHead();

  /* set default behavior */
  user_input_fd = stdin;
  user_output_fd = stdout;
  ctm_input_fd = NULL;
  ctm_output_fd = NULL;
  compat_flag = 0;
  baudot_flag = 0;
  negotiation_flag = 0;
  ctm_file_mode_flag = 0;
  audio_mode_flag = 1;

  int ch;
  while ((ch = getopt(argc, argv, "cbni:o:f:I:O:")) != -1) {
    switch (ch) {
      case 'c':
        compat_flag = 1;
        break;
      case 'b':
        baudot_flag = 1;
        break;
      case 'n':
        negotiation_flag = 1;
        break;
      case 'I':
        ctm_file_mode_flag = 1;
        audio_mode_flag = 0;
        ctm_input_fd = open_file_or_stdio(optarg);
        break;
      case 'O':
        ctm_file_mode_flag = 1;
        audio_mode_flag = 0;
        ctm_output_fd = open_file_or_stdio(optarg);
        break;
      case 'i':
        user_input_fd = open_file_or_stdio(optarg);
        break;
      case 'o':
        user_output_fd = open_file_or_stdio(optarg);
        break;
      default:
        usage();
        /* NOTREACHED */
    }
  }
  argc -= optind;
  argv += optind;

  /* check for sane argument combinations */
  if (audio_mode_flag == 1 && ctm_file_mode_flag == 1)
    errx(1, "invalid arguments: if using audio mode, no CTM files can be specified.");

  else if (ctm_file_mode_flag == 1 && (ctm_input_fd == NULL || ctm_output_fd == NULL))
    errx(1, "invalid arguments: both CTM input and output files must be specified.");

  else if (baudot_flag == 1 && (user_input_fd == NULL || user_output_fd == NULL))
    errx(1, "invalid arguments: both baudot input and output files must be specified in baudot mode.");

  else if (compat_flag == 1 && ctm_file_mode_flags == 0 && baudot_flag == 0)
    errx(1, "invalid arguments: compatibility mode is used only with baudot and/or CTM file modes.");

  /* select the user input mode and CTM mode based on the input arguments. */
  if (ctm_file_mode_flag == 1) {
    if (compat_flag == 0)
      ctm_mode = CTM_FILE;
    else
      ctm_mode = CTM_fILE_COMPAT;
  }
  else
    ctm_mode = CTM_AUDIO;

  if (baudot_flag == 1) {
    if (compat_flag == 0)
      user_input_mode = CTM_BAUDOT_IN;
    else
      user_input_mode = CTM_BAUDOT_IN_COMPAT;
  }
  else
    user_input_mode = CTM_TEXT_IN;

  /**************************************************************/
  /* Main processing loop                                       */
  /**************************************************************/

  //ctm_init
  //ctm_set_negotiation(negotiation_flag)
  //ctm_start

  do
    {
#ifdef PCAUDIO /* read samples from audio-I/O, if desired */
      if (ctm_audio_dev_mode)
      {
        /* ctm_input_buffer == CTM input samples */
        size_t bytes_left_to_read = LENGTH_TONE_VEC*sizeof(Shortint);

        /* block until we have enough input samples. */
        while (bytes_left_to_read > 0)
        {
          bytes_left_to_read -= sio_read(audio_hdl, ctm_input_buffer + bytes_left_to_read*sizeof(Shortint), bytes_left_to_read);
        }
      }
#endif
      /* Read next frame of input samples from files */
      if (baudotReadFromFile)
      {
        if(fread(baudot_input_buffer, sizeof(Shortint), 
                 LENGTH_TONE_VEC, baudotInputFileFp) < LENGTH_TONE_VEC)
          {
            /* if EOF is reached, use buffer with zeros instead */
            baudotEOF = true;
            for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
              baudot_input_buffer[cnt]=0;
          }

#ifdef LSBFIRST
        else if (compat_mode)
        {
                /* The test pattern baudot PCM files are in big-endian. If we are on a little-endian machine, we will need to swap the bytes */
                for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
                {
                        baudot_input_buffer[cnt] = swap16(baudot_input_buffer[cnt]);
                }
        }
#endif
      }
      
      if (ctmReadFromFile)
      {
        if(fread(ctm_input_buffer, sizeof(Shortint), 
                 LENGTH_TONE_VEC, ctmInputFileFp) < LENGTH_TONE_VEC)
          {
            /* if EOF is reached, use buffer with zeros instead */
            ctmEOF = true;
            for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
              ctm_input_buffer[cnt]=0;
          }

#ifdef LSBFIRST
        if (compat_mode)
        {
                /* The test pattern baudot PCM files are in big-endian. If we are on a little-endian machine, we will need to swap the bytes */
                for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
               {
                        ctm_input_buffer[cnt] = swap16(ctm_input_buffer[cnt]);
                }
        }
#endif
      }
      
      /* As long as the Baudot Code Modulator is active: Mute the Baudot   */
      /* input signal in order to get rid of the echoes from the PSTN side */
      if (numBaudotBitsStillToModulate>0)
        for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
          baudot_input_buffer[cnt] = 0;
      
      /* Run the Baudot demodulator */
      baudot_tonedemod(baudot_input_buffer, LENGTH_TONE_VEC, 
                       &baudotOutTTYCodeFifoState, &baudot_tonedemod_state);
      /* Adjust the Mode of the modulator according to the demodulator */
      baudot_tonemod_state.inFigureMode = baudot_tonedemod_state.inFigureMode;
      
      /* Set flag indicating that the demodulator has already */
      /* decoded a Baudot character.                          */ 
      if(Shortint_fifo_check(&baudotOutTTYCodeFifoState)>0)
        baudotAlreadyReceived = true;
      
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
      
      if (baudot_tonedemod_state.cntBitsActualChar>=5)
        {
          if (baudotAlreadyReceived)
            actualBaudotCharDetected = true;
          else
            actualBaudotCharDetected = (baudot_tonedemod_state.ttyCode>0);
        }
      else
        actualBaudotCharDetected = false;
      
      /* The next lines guarantee that the Baudot signal is muted even */
      /* if the received character is a SHIFT symbol (a SHIFT symbol   */
      /* would not set the CTM transmitter into an active state).      */ 
      if (cntHangoverFramesForMuteBaudot>0) 
        cntHangoverFramesForMuteBaudot--;
      if (actualBaudotCharDetected)
        cntHangoverFramesForMuteBaudot = 1+(320/LENGTH_TONE_VEC);

      /* Run the CTM receiver */
      
      Shortint_fifo_push(&signalFifoState,ctm_input_buffer, 
                         LENGTH_TONE_VEC);
      
      ctm_receiver(&signalFifoState, &ctmOutTTYCodeFifoState, 
                   &earlyMutingRequired, &rx_state);
      
      enquiryFromFarEndDetected = false;
      
      /* Check whether the far-end side is able to support CTM signals */
      if ((rx_state.wait_state.sync_found) && (!ctmFromFarEndDetected))
        {
          ctmFromFarEndDetected = true;
          fprintf(stderr, ">>> CTM from far-end detected! <<<\n");
          
          /* If we have not transmitted CTM tones so far, we should */
          /* treat the received burst as an enquiry burst.          */
          if (!ctmCharacterTransmitted)
            {
              enquiryFromFarEndDetected=true;
              cntFramesSinceEnquiryDetected=0;
            }
        }
      
      /************ First, we process the transmission from the ***********/
      /************ CTM receiver to the Baudot modulator.       ***********/
      
      /* If there is a character from the CTM receiver available          */
      /* --> print it on the screen and push it into ctmToBaudotFifo.     */
      if (Shortint_fifo_check(&ctmOutTTYCodeFifoState) >0)
        {
          Shortint_fifo_pop(&ctmOutTTYCodeFifoState, &ucsCode, 1);
          
          /* Check whether this was an enquiry burst from the other */
          /* side. Ignore this enquiry, if the last enquiry has     */
          /* been detected less than 25 frames (500 ms) ago.        */
          if ((ucsCode==ENQU_SYMB) && 
              (cntFramesSinceEnquiryDetected > 25*160/LENGTH_TONE_VEC))
            {
              enquiryFromFarEndDetected=true;
              cntFramesSinceEnquiryDetected=0;
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
                  if (writeToTextFile)
                    fprintf(textOutputFileFp, "%c", character);
                  Shortint_fifo_push(&ctmToBaudotFifoState, &ttyCode, 1);
                }
            }
        }
      
      if (enquiryFromFarEndDetected)
        fprintf(stderr,">>> Enquiry From Far End Detected! <<<\n");
      
      if (cntFramesSinceEnquiryDetected<maxShortint)
        cntFramesSinceEnquiryDetected++;
      
      /* If there are characters from the CTM receiver, or if the CTM     */
      /* receiver has detected a synchronisation preamble, or if the      */
      /* Baudot Modulator is still busy (i.e. there are still bits to     */
      /* modulate) --> run the Baudot Modulator (again). This branch is   */
      /* also executed (but with the Baudot Modulator in idle mode), if   */
      /* the ctm_receiver has indicated that early_muting is required.    */
      if ((Shortint_fifo_check(&ctmToBaudotFifoState) >0) || 
          (numBaudotBitsStillToModulate>0) || earlyMutingRequired)
        {
          /* If there is a character on the ctmToBaudotFifo available and */
          /* if the Baudot modulator is able to process a new character   */
          /* --> pop the character from the fifo.                         */
          if ((Shortint_fifo_check(&ctmToBaudotFifoState) >0) &&
              (numBaudotBitsStillToModulate <= 8) &&
              (cntFramesSinceLastBypassFromCTM>=10*160/LENGTH_TONE_VEC))
            Shortint_fifo_pop(&ctmToBaudotFifoState, &ttyCode, 1);
          else
            ttyCode = -1;
          
          baudot_tonemod(ttyCode, baudot_output_buffer, LENGTH_TONE_VEC,
                         &numBaudotBitsStillToModulate,
                         &baudot_tonemod_state);
          /* Adjust the Mode of the demodulator according to the modulator */
          baudot_tonedemod_state.inFigureMode 
            = baudot_tonemod_state.inFigureMode;
          
          if (cntFramesSinceLastBypassFromCTM<maxShortint)
            cntFramesSinceLastBypassFromCTM++;
        }
      else
        {
          /* If the modulator has not been run                   */
          /* --> copy samples from the input to the output       */
          /*     or set them to zero, if bypassing is prohibited */
          if (!disableBypass)
            for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
              baudot_output_buffer[cnt] = ctm_input_buffer[cnt];
          else
            for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
              baudot_output_buffer[cnt] = 0;
          cntFramesSinceLastBypassFromCTM = 0;
        }
      
      /************ Now we process the transmission from the    *************/
      /************ Baudot demodulator to the CTM transmitter   *************/
      
      if (enquiryFromFarEndDetected)
        {
          /* Generate Acknowledgement burst, if Enquiry from the far side */
          /* has been detected. The code 0xFFFF has a special meaning,    */
          /* see description of function ctm_transmitter()                */
          if (Shortint_fifo_check(&baudotToCtmFifoState)==0)
            {
              ucsCode = 0xFFFF;
              Shortint_fifo_push(&baudotToCtmFifoState, &ucsCode, 1);
              ctmCharacterTransmitted   = true;
              enquiryFromFarEndDetected = false;
            }
        }
      else if (!ctmFromFarEndDetected && ctmTransmitterIsIdle &&
               (Shortint_fifo_check(&baudotOutTTYCodeFifoState)>0) &&
               (cntTransmittedEnquiries<NUM_ENQUIRY_BURSTS))
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
          Shortint_fifo_push(&baudotToCtmFifoState, &ucsCode, 1);
          ctmCharacterTransmitted = true;
          if (cntTransmittedEnquiries<maxShortint)
            cntTransmittedEnquiries++;
        }
      
      /* The CTM transmitter is executed in the following cases:            */
      /* - Character from Baudot demodulator available or start + info bits */
      /*   detected, provided that we know that the other side supports CTM */
      /*   or if we are still in the negotiation phase                      */
      /* - There is still a character in the baudotToCtmFifo                */
      /* - The CTM Modulator is still running                               */
      /* Otherwise, the audio samples are bypassed!                         */
      
      if((((Shortint_fifo_check(&baudotOutTTYCodeFifoState)>0) || 
           (actualBaudotCharDetected || (cntHangoverFramesForMuteBaudot>0) ) || read_from_text_file) && 
          (ctmFromFarEndDetected || (cntFramesSinceBurstInit<ENQUIRY_TIMEOUT))) ||
         (Shortint_fifo_check(&baudotToCtmFifoState)>0) || 
         (numCTMBitsStillToModulate>0) || tx_state.burstActive ||
         ((cntHangoverFramesForMuteBaudot>0)
          &&(cntTransmittedEnquiries<NUM_ENQUIRY_BURSTS)))
        {
          /* Transition from idle into active mode represents start of burst */
          if (ctmTransmitterIsIdle)
            cntFramesSinceBurstInit = 0;
          
          /* If there is a character from the Baudot receiver available     */
          /* and if the CTM transmitter is able to process a new character: */
          /* pop the character from the fifo.                               */
          
          if ((ctmFromFarEndDetected || (cntFramesSinceBurstInit<ENQUIRY_TIMEOUT)) &&
              (Shortint_fifo_check(&baudotToCtmFifoState)==0))
          {
            if (Shortint_fifo_check(&baudotOutTTYCodeFifoState)>0)
            {
              Shortint_fifo_pop(&baudotOutTTYCodeFifoState, &ttyCode, 1);
              character = convertTTYcode2char(ttyCode);
              fprintf(stderr, "%c", character);
              ucsCode = convertChar2UCScode(character);
              Shortint_fifo_push(&baudotToCtmFifoState, &ucsCode, 1);
            }

            /* otherwise we are reading from a text file. */
            else if (read_from_text_file)
            {
              if (fread(&character, sizeof(character), 1, text_input_file_fp) > 0)
              {
                fprintf(stderr, "%c", character);
                ucsCode = convertChar2UCScode(character);
                Shortint_fifo_push(&baudotToCtmFifoState, &ucsCode, 1);
              }
            }
          }
          
          if ((Shortint_fifo_check(&baudotToCtmFifoState)>0) &&
              (numCTMBitsStillToModulate<2*LENGTH_TX_BITS))
            Shortint_fifo_pop(&baudotToCtmFifoState, &ucsCode, 1);
          else
            ucsCode = 0x0016;
          
          ctm_transmitter(ucsCode, ctm_output_buffer, &tx_state, 
                          &numCTMBitsStillToModulate, sineOutput);
          
          ctmTransmitterIsIdle    
            = !tx_state.burstActive && (numCTMBitsStillToModulate==0);
          ctmCharacterTransmitted = true;
          syncOnBaudot = false;
        }
      else
        {
          /* After terminating a CTM burst, the forwarding of the original */
          /* audio signal might have to be delayed if there is actually    */
          /* a Baudot character in progress. In this case, we have to mute */
          /* the output signal until the character is completed.           */
          /* The audio signal is also muted if bypassing has been          */
          /* prohibited by the user (i.e. by using the option -nobypass)   */
          if ((!syncOnBaudot && 
               (baudot_tonedemod_state.cntBitsActualChar>0)) || disableBypass)
            for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
              ctm_output_buffer[cnt] = 0;
          else
            {
              /* Bypass audio samples. */
              for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
                ctm_output_buffer[cnt] = baudot_input_buffer[cnt];
              syncOnBaudot = true;
            }
          
          /* discard characters in oder to avoid FIFO buffer overflows */
          if (Shortint_fifo_check(&baudotOutTTYCodeFifoState)>=
              baudotOutTTYCodeFifoLength-1)
            Shortint_fifo_pop(&baudotOutTTYCodeFifoState, &ttyCode, 1);
        }
            
      if (cntFramesSinceBurstInit<maxShortint)
        cntFramesSinceBurstInit++;
      
      if (baudotWriteToFile)
      {
#ifdef LSBFIRST
        /* The test pattern baudot PCM files are in big-endian. If we are on a little-endian machine, we will need to swap the bytes */
        if (compat_mode)
        {
          for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
          {
                  baudot_output_buffer[cnt] = swap16(baudot_output_buffer[cnt]);
          }
        }
#endif
        if (fwrite(baudot_output_buffer, sizeof(Shortint), 
                   LENGTH_TONE_VEC, baudotOutputFileFp) < LENGTH_TONE_VEC)
          {
            errx(1, "error while writing to '%s'\n", 
                    baudotOutputFileName);
          }
      }
      
#ifdef PCAUDIO /* write samples to audio-I/O, if desired */
      if (ctm_audio_dev_mode)
      {
        /* ctm_output_buffer == CTM output samples */
        size_t bytes_left_to_write = LENGTH_TONE_VEC*sizeof(Shortint);

        /* block until we have enough input samples. */
        while (bytes_left_to_write > 0)
        {
          bytes_left_to_write -= sio_write(audio_hdl, ctm_output_buffer + bytes_left_to_write*sizeof(Shortint), bytes_left_to_write);
        }
      }
#endif

      if (ctmWriteToFile)
      {
#ifdef LSBFIRST
        /* The test pattern baudot PCM files are in big-endian. If we are on a little-endian machine, we will need to swap the bytes */
        if (compat_mode)
        {
          for (cnt=0; cnt<LENGTH_TONE_VEC; cnt++)
          {
                  ctm_output_buffer[cnt] = swap16(ctm_output_buffer[cnt]);
          }
        }
#endif
        if (fwrite(ctm_output_buffer, sizeof(Shortint), 
                   LENGTH_TONE_VEC, ctmOutputFileFp) < LENGTH_TONE_VEC)
          {
            errx(1, "error while writing to '%s'\n", 
                    ctmOutputFileName);
          }
      }
      
      /* The loop is exited, if both input files do not contain any */
      /* samples and if both transmitters are no longer busy.       */
      /* Alternatively, the loop is also exited, if the number of   */
      /* processed samples is greater than the predefined limit     */
      
      cntProcessedSamples+=LENGTH_TONE_VEC;
      if ((cntProcessedSamples >= numSamplesToProcess) ||
          (baudotEOF && ctmEOF && ctmTransmitterIsIdle && 
           (Shortint_fifo_check(&ctmToBaudotFifoState)==0) && 
           (numBaudotBitsStillToModulate==0)))
        break;
    }
  while (1);
  
  fprintf(stderr, "\n");
  fclose(ctmInputFileFp);
  fclose(baudotInputFileFp);
  fclose(ctmOutputFileFp);
  fclose(baudotOutputFileFp);
  
  if (writeToTextFile)
    fclose(textOutputFileFp);

#ifdef PCAUDIO
  if (ctm_audio_dev_mode)
  {
    sio_close(audio_hdl);
  }
#endif

  exit(0);
}
