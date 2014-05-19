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

#include "ctm_defines.h"
#include "ctm_transmitter.h"
#include "ctm_receiver.h"
#include "baudot_functions.h"
#include "ucs_functions.h"
#include <typedefs.h>
#include <fifo.h>
#include <parse_arg.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#include <ctype.h>      /* toupper() */
#include <termios.h>

#ifdef PC
//#define PCAUDIO
#endif

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

void errorMsg(const char* filename)
{
  fprintf(stderr, "                     +------------+                 \n");
  fprintf(stderr, "  Baudot Tones  ---> |            | ---> CTM signal \n");
  fprintf(stderr, "                     | adaptation |                 \n");
  fprintf(stderr, "  Baudot Tones  <--- |            | <--- CTM signal \n");
  fprintf(stderr, "                     +------------+                 \n");
  fprintf(stderr, "use: %s [ arguments ]\n", filename);
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

FILE* open_input_file_or_stdin(const char *filename)
{
  FILE *file_fd;
  if (!strncmp(filename, "-", 1))
  {
    file_fd = stdin;
  }
  else
  {
    file_fd = fopen(filename, "r");
    if (file_fd == (FILE*)NULL)
    {
      errx(1, "can't open input file '%s'\n", filename);
    }
  }

  return file_fd;
}

FILE* open_output_file_or_stdout(const char *filename)
{
  FILE *file_fd;
  if (!strncmp(filename, "-", 1))
  {
    file_fd = stdout;
  }
  else
  {
    file_fd = fopen(filename, "w");
    if (file_fd == (FILE*)NULL)
    {
      errx(1, "can't open output file '%s'\n", filename);
    }
  }

  return file_fd;
}
/***********************************************************************/

int main(int argc, const char** argv)
{
  Shortint     cnt;
  int          optindex;

  Shortint     numCTMBitsStillToModulate       = 0;
  Shortint     numBaudotBitsStillToModulate    = 0;
  Shortint     cntFramesSinceBurstInit         = 0;
  Shortint     cntFramesSinceLastBypassFromCTM = 0;
  Shortint     cntFramesSinceEnquiryDetected   = 0;
  Shortint     cntTransmittedEnquiries         = 0;
  Shortint     cntHangoverFramesForMuteBaudot  = 0;
  
  Shortint     ttyCode                         = 0;
  UShortint    ucsCode                         = 0;
  
  ULongint     cntProcessedSamples             = 0;
  ULongint     numSamplesToProcess             = maxULongint;
  
  char         character;
      
  Shortint     baudot_output_buffer[LENGTH_TONE_VEC];
  Shortint     ctm_output_buffer[LENGTH_TONE_VEC];

  Bool         sineOutput                = false;
  Bool         writeToTextFile           = false;
  Bool         read_from_text_file       = false;
  
  Bool         ctmReadFromFile           = false;
  Bool         baudotReadFromFile        = false;
  Bool         ctmWriteToFile            = false;
  Bool         baudotWriteToFile         = false;
  
  Bool         ctmFromFarEndDetected     = false;
  Bool         ctmCharacterTransmitted   = false;
  Bool         enquiryFromFarEndDetected = false;
  Bool         syncOnBaudot              = true;
  Bool         ctmTransmitterIsIdle      = true;
  Bool         baudotEOF                 = false;
  Bool         ctmEOF                    = false;
  Bool         disableNegotiation        = false;
  Bool         disableBypass             = false;
  Bool         earlyMutingRequired       = false;
  Bool         baudotAlreadyReceived     = false;
  Bool         actualBaudotCharDetected  = false;

  Bool         compat_mode               = false;
  Bool         ctm_audio_dev_mode        = true; /* by default, we will use the default system audio device for CTM I/O. */
  
  tx_state_t   tx_state;
  rx_state_t   rx_state;
  
  /* State variables for the Baudot demodulator and modulator,respectively. */
  
  baudot_tonedemod_state_t baudot_tonedemod_state;
  baudot_tonemod_state_t   baudot_tonemod_state;
  
  /* Define fifo state variables */
  
  Shortint      baudotOutTTYCodeFifoLength = 50;
  fifo_state_t  baudotOutTTYCodeFifoState;
  fifo_state_t  signalFifoState;
  fifo_state_t  ctmOutTTYCodeFifoState;
  fifo_state_t  baudotToCtmFifoState;
  fifo_state_t  ctmToBaudotFifoState;

  Shortint   baudot_input_buffer[LENGTH_TONE_VEC];
  Shortint   ctm_input_buffer[LENGTH_TONE_VEC];

  /* Define file variables */
  
  FILE  *ctmInputFileFp     = NULL;
  FILE  *baudotInputFileFp  = NULL;
  FILE  *ctmOutputFileFp    = NULL;
  FILE  *baudotOutputFileFp = NULL;
  FILE  *textOutputFileFp   = stdout;
  FILE  *text_input_file_fp = stdin;
  
  const char* ctmInputFileName     = NULL;
  const char* baudotInputFileName  = NULL;
  const char* ctmOutputFileName    = NULL;
  const char* baudotOutputFileName = NULL;
  const char* textOutputFileName   = NULL;
  const char* text_input_filename  = NULL;
  const char* audio_device_name    = SIO_DEVANY;
  
#ifdef PCAUDIO /* only if real-time audio I/O is desired */
  struct sio_hdl *audio_hdl = NULL;
  struct sio_par audio_params; 

  sio_initpar(&audio_params);
  audio_params.rate = 8000;
  audio_params.bits = 16;
  audio_params.bps = 2;
  audio_params.le = SIO_LE_NATIVE;
  audio_params.rchan = 1;
  audio_params.pchan = 1;
  audio_params.appbufsz = LENGTH_TONE_VEC;
  
  /* for over/underrun testing. */
  //audio_params.xrun = SIO_ERROR;

#endif
  
  writeHead();
  
  /* parse the argument line */
  for (cnt=1; cnt<argc; cnt++)
    {
      optindex = get_option(argc, argv, cnt, 11, "-h", 
                            "-ctmin", "-ctmout", "-baudotin", "-baudotout", 
                            "-textout", "-numsamples", 
                            "-nonegotiation", "-nobypass", "-compat", "-textin");
      if (optindex<0)
        break;
      
      switch (optindex)
        {
        case 0:
          errorMsg(argv[0]);
          break;
        case 1:
          ctmReadFromFile = true;
          cnt++;
          ctmInputFileName = get_argument(argc, argv, cnt);
          if (ctmInputFileName==(char*)NULL)
            {
              fprintf(stderr, "\nerror: undetermined ctm input filename!\n");
              errorMsg(argv[0]);
            }
          break;
        case 2:
          ctmWriteToFile = true;
          cnt++;
          ctmOutputFileName = get_argument(argc, argv, cnt);
          if (ctmOutputFileName==(char*)NULL)
            {
              fprintf(stderr, "\nerror: undetermined ctm output filename!\n");
              errorMsg(argv[0]);
            }
          break;
        case 3:
          baudotReadFromFile = true;
          cnt++;
          baudotInputFileName = get_argument(argc, argv, cnt);
          if (baudotInputFileName==(char*)NULL)
            {
              fprintf(stderr, "\nerror: undetermined baudot input text filename!\n");
              errorMsg(argv[0]);
            }
          break;
        case 4: 
          baudotWriteToFile = true;
          cnt++;
          baudotOutputFileName = get_argument(argc, argv, cnt);
          if (baudotOutputFileName==(char*)NULL)
            {
              fprintf(stderr, "\nerror: undetermined baudot output filename!\n");
              errorMsg(argv[0]);
            }
          break;
        case 5:
          cnt++;
          writeToTextFile    = true;
          textOutputFileName = get_argument(argc, argv, cnt);
          if (textOutputFileName==(char*)NULL)
            {
              fprintf(stderr, "\nerror: undetermined text output filename!\n");
              errorMsg(argv[0]);
            }
          break;
        case 6:
          cnt++;
          if (get_argument(argc, argv, cnt)==(char*)NULL)
            {
              fprintf(stderr, "\nerror: number of samples to process not specified!\n");
              errorMsg(argv[0]);
            }
          numSamplesToProcess = atol(get_argument(argc, argv, cnt));
          break;
        case 7:
          disableNegotiation = true;
          break;
        case 8:
          disableBypass = true;
          break;
        case 9:
          compat_mode = true;
          break;
        case 10:
          cnt++;
          read_from_text_file = true;

          if (baudotReadFromFile)
          {
            /* mutually exclusive inputs were specified. */
            fprintf(stderr, "\nwarning: baudot input and text input were both specified. Ignoring baudot input.\n");
            baudotReadFromFile = false;
          }

          text_input_filename = get_argument(argc, argv, cnt);
          if (text_input_filename==(char*)NULL)
          {
              fprintf(stderr, "\nerror: undetermined text input filename!\n");
              errorMsg(argv[0]);
          }
          break;
        default:
          fprintf(stderr, "\nerror: unknown option: %s\n", argv[cnt]);
          errorMsg(argv[0]);
        }
    }

  /* check if there are still unconsumed (i.e. invalid) arguments */
  if (get_argument(argc, argv, cnt)!=(char*)NULL)
    {
      fprintf(stderr, "Too much or invalid arguments!\n\n");
      errorMsg(argv[0]);
    }

  /* if any CTM files were specified, do not use the audio device. */
  if (ctmReadFromFile || ctmWriteToFile)
  {
    ctm_audio_dev_mode = false;
  }

  /* check whether all neccessary filenames are specified */
  if (ctmReadFromFile && (ctmInputFileName==NULL))
    {
      fprintf(stderr, "CTM input filename not specified!\n\n");
      errorMsg(argv[0]);
    }
  if (baudotReadFromFile && (baudotInputFileName==NULL))
    {
      fprintf(stderr, "Baudot Tones input filename not specified!\n\n");
      errorMsg(argv[0]);
    }
  if (ctmWriteToFile && (ctmOutputFileName==NULL))
    {
      fprintf(stderr, "CTM output filename not specified!\n\n");
      errorMsg(argv[0]);
    }
  if (baudotWriteToFile && (baudotOutputFileName==NULL))
    {
      fprintf(stderr, "Baudot Tones output filename not specified!\n\n");
      errorMsg(argv[0]);
    }
  
  /* open all required files for signal input and output */
  if (ctmReadFromFile)
    {
      ctmInputFileFp = open_input_file_or_stdin(ctmInputFileName);
    }
  if (ctmWriteToFile)
    {
      ctmOutputFileFp = open_output_file_or_stdout(ctmOutputFileName);
    }
  if (baudotReadFromFile)
    {
      baudotInputFileFp = open_input_file_or_stdin(baudotInputFileName);
    }
  if (baudotWriteToFile)
    {
      baudotOutputFileFp = open_output_file_or_stdout(baudotOutputFileName);
    }
  if (writeToTextFile)
    {
      textOutputFileFp = open_output_file_or_stdout(textOutputFileName);
    }
  if (read_from_text_file)
  {
    text_input_file_fp = open_input_file_or_stdin(text_input_filename);
  }
#ifdef PCAUDIO /* if desired, open audio device for duplex i/o */
  if (ctm_audio_dev_mode)
  {
    /* open in blocking I/O mode, as that appears to be what the original 3GPP code expects in its processing loop. */
    fprintf(stderr, "opening audio device for duplex i/o...\n");
    audio_hdl = sio_open(audio_device_name, SIO_PLAY | SIO_REC, 0);
    if (audio_hdl == NULL)
      {
        errx(1, "unable to open audio device \"%s\" for duplex i/o\n", audio_device_name);
      }

    /* attempt to set the device parameters. */
    if (sio_setpar(audio_hdl, &audio_params) == 0)
    {
      errx(1, "unable to set device parameters on audio device \"%s\"\n", audio_device_name);
    }

    /* check to see that the device parameters were actually set up correctly. Not all devices may support the required parameters. */
    struct sio_par dev_params;
    if (sio_getpar(audio_hdl, &dev_params) == 0)
    {
      errx(1, "unable to get device parameters on audio device \"%s\"\n", audio_device_name);
    }

    else if ((audio_params.rate != dev_params.rate) || (audio_params.bits != dev_params.bits) || (audio_params.rchan != dev_params.rchan) || (audio_params.pchan != dev_params.pchan) || (audio_params.appbufsz != dev_params.appbufsz))

    {
      errx(1, "unable to set the correct parameters on audio device \"%s\"\n", audio_device_name);
    } 
  }
#endif
  
  /* set up transmitter & receiver */
  init_baudot_tonedemod(&baudot_tonedemod_state);
  init_baudot_tonemod(&baudot_tonemod_state);
  init_ctm_transmitter(&tx_state);
  init_ctm_receiver(&rx_state);  

  Shortint_fifo_init(&signalFifoState, SYMB_LEN+LENGTH_TONE_VEC);
  Shortint_fifo_init(&baudotOutTTYCodeFifoState, baudotOutTTYCodeFifoLength);
  Shortint_fifo_init(&ctmOutTTYCodeFifoState,  2);
  Shortint_fifo_init(&ctmToBaudotFifoState,  4000);
  Shortint_fifo_init(&baudotToCtmFifoState,  3);
  
  /* zero all the I/O buffers */
  for(cnt=0;cnt<LENGTH_TONE_VEC;cnt++)
  {
    ctm_input_buffer[cnt] = 0;
    baudot_input_buffer[cnt] = 0;
    ctm_output_buffer[cnt] = 0;
    baudot_output_buffer[cnt] = 0;
  }

  if (numSamplesToProcess < maxULongint)
    fprintf(stderr, "number of samples to process: %u\n\n", numSamplesToProcess);
  
#ifdef PCAUDIO
  if (ctm_audio_dev_mode)
  {
    fprintf(stderr, "starting audio device \"%s\"...\n", audio_device_name);
    if(sio_start(audio_hdl) == 0)
    {
      errx(1, "unable to start audio device \"%s\".\n", audio_device_name);
    }

    if(sio_setvol(audio_hdl, SIO_MAXVOL) == 0)
    {
      errx(1, "unable to set audio volume on device \"%s\".\n", audio_device_name);
    }

    /* we have to write to the output buffer first or reading will block indefinitely. */
    for(cnt=0;cnt<2880/LENGTH_TONE_VEC;cnt++)
      sio_write(audio_hdl, ctm_output_buffer, LENGTH_TONE_VEC);
  } 
#endif

  if (disableNegotiation)
    ctmFromFarEndDetected = true;

  /**************************************************************/
  /* Main processing loop                                       */
  /**************************************************************/

  do
    {
#ifdef PCAUDIO /* read samples from audio-I/O, if desired */
      if (ctm_audio_dev_mode)
      {
        /* ctm_input_buffer == CTM input samples */
        size_t bytes_read = 0;

        /* block until we have enough input samples. */
        while (bytes_read < LENGTH_TONE_VEC)
        {
          bytes_read += sio_read(audio_hdl, ctm_input_buffer + bytes_read, LENGTH_TONE_VEC);
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
        else if (compat_mode)
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
                ctm_output_buffer[cnt] = 0; //baudot_input_buffer[cnt];
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
        size_t bytes_written = 0;

        /* block until we have enough input samples. */
        while (bytes_written < LENGTH_TONE_VEC)
        {
          bytes_written += sio_write(audio_hdl, ctm_output_buffer + bytes_written, LENGTH_TONE_VEC);
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

  return 0;
}
