#include <poll.h>

#include "ctm.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#ifdef __OpenBSD__
#include <sys/types.h>
#include <sndio.h>
#define PCAUDIO
#endif

#include <ctype.h>
#include <termios.h>

#include "ctm_defines.h"
#include "ctm_transmitter.h"
#include "ctm_receiver.h"
#include "baudot_functions.h"
#include "ucs_functions.h"
#include <typedefs.h>
#include <fifo.h>

static struct ctm_state state;

static void
set_modes(enum ctm_output_mode ctm_output_mode, enum ctm_user_input_mode input_mode, int ctm_input_fd, int ctm_output_fd, int user_input_fd, int user_output_fd)
{
  switch(output_mode) {
    case CTM_AUDIO:
      /* this is the default mode. */
      state->audio_dev_name = ctm_input_name; 
      break;
    case CTM_FILE:
      state->ctmReadFromFile           = true;
      state->ctmWriteToFile            = true;
      state->ctm_audio_dev_mode        = false;
      state->ctmInputFileName          = ctm_input_name;
      state->ctmOutputFileName         = ctm_output_name;

      state->ctmInputFp = open_file_or_stdio(ctm_input_name, CTM_IO_IN);
      state->ctmOutputFp = open_file_or_stdio(ctm_input_name, CTM_IO_OUT);
      break;
    case CTM_FILE_COMPAT:
      state->ctmReadFromFile           = true;
      state->ctmWriteToFile            = true;
      state->ctm_audio_dev_mode        = false;
      state->ctmInputFileName          = user_input_name;
      state->ctmOutputFileName         = user_output_name;
      state->compat_mode               = true;

      state->ctmInputFp = open_file_or_stdio(ctm_input_name, CTM_IO_IN);
      state->ctmOutputFp = open_file_or_stdio(ctm_input_name, CTM_IO_OUT);
      break;
    default:
      errx(1, "invalid CTM mode.");
      break;
  }

  switch(input_mode) {
    case CTM_TEXT_IN:
      /* this is the default user input mode. */
      state->userInputFp = open_file_or_stdio(user_input_name, CTM_IO_IN);
      state->userOutputFp = open_file_or_stdio(user_output_name, CTM_IO_OUT);
      break;
    case CTM_BAUDOT_IN:
      state->writeToTextFile           = false;
      state->read_from_text_file       = false;
      state->baudotReadFromFile        = true;
      state->baudotWriteToFile         = true;
      state->userInputFp = open_file_or_stdio(user_input_name, CTM_IO_IN);
      state->userOutputFp = open_file_or_stdio(user_output_name, CTM_IO_OUT);
      break;
    case CTM_BAUDOT_IN_COMPAT:
      state->writeToTextFile           = false;
      state->read_from_text_file       = false;
      state->baudotReadFromFile        = true;
      state->baudotWriteToFile         = true;
      state->compat_mode               = true;
      state->userInputFp = open_file_or_stdio(user_input_name, CTM_IO_IN);
      state->userOutputFp = open_file_or_stdio(user_output_name, CTM_IO_OUT);
      break;
    default:
      errx(1, "invalid user input mode.");
      break;
  }
}

void
open_audio_devices(void)
{
  if (state->ctm_audio_dev_mode)
  {
    /* open in blocking I/O mode, as that appears to be what the original 3GPP code expects in its processing loop. */
    fprintf(stderr, "opening audio device for duplex i/o...\n");
    state->audio_hdl = sio_open(state->audio_device_name, SIO_PLAY | SIO_REC, 0);
    if (state->audio_hdl == NULL)
    {
      errx(1, "unable to open audio device \"%s\" for duplex i/o\n", state->audio_device_name);
    }

    /* attempt to set the device parameters. */
    if (sio_setpar(state->audio_hdl, &state->audio_params) == 0)
    {
      errx(1, "unable to set device parameters on audio device \"%s\"\n", state->audio_device_name);
    }

    /* check to see that the device parameters were actually set up correctly. Not all devices may support the required parameters. */
    struct sio_par dev_params;
    if (sio_getpar(state->audio_hdl, &dev_params) == 0)
    {
      errx(1, "unable to get device parameters on audio device \"%s\"\n", state->audio_device_name);
    }

    else if ((state->audio_params.rate != dev_params.rate) || (state->audio_params.bits != dev_params.bits) || (state->audio_params.rchan != dev_params.rchan) || (state->audio_params.pchan != dev_params.pchan) || (state->audio_params.appbufsz != dev_params.appbufsz))

    {
      errx(1, "unable to set the correct parameters on audio device \"%s\"\n", state->audio_device_name);
    } 
  }
}

/* enable/disable CTM negotiation (ENQUIRIES). */
void
ctm_set_negotiation(enum on_off flag)
{
  switch(flag) {
    case ON:
      state->disableNegotiation = false;
      break;
    case OFF:
      state->disableNegotiation = true;
      break;
    default:
      errx(1, "invalid set_negotiation flag.");
  }
}

void
ctm_init(enum ctm_output_mode output_mode, enum ctm_user_input_mode input_mode, char *ctm_output_name, char *ctm_input_name, char *user_output_name, char *user_input_name)
{
  /* initialize the ctm_state structure here. */
  state = calloc(1, sizeof(struct ctm_state));

  /* setup some defaults */
  state->numSamplesToProcess           = maxULongint;
  state->compat_mode                   = false;
  state->ctmReadFromFile               = false;
  state->ctmWriteToFile                = false;
  state->ctm_audio_dev_mode            = true;
  state->writeToTextFile               = true;
  state->read_from_text_file           = true;
  state->baudotReadFromFile            = false;
  state->baudotWriteToFile             = false;
  state->ctmInputFileFp                = NULL;
  state->baudotInputFileFp             = NULL;
  state->ctmOutputFileFp               = NULL;
  state->baudotOutputFileFp            = NULL;
  state->textOutputFileFp              = stdout;
  state->textInputFileFp               = stdin;

  state->ctmInputFileName              = NULL;
  state->baudotInputFileName           = NULL;
  state->ctmOutputFileName             = NULL;
  state->baudotOutputFileName          = NULL;
  state->textOutputFileName            = NULL;
  state->text_input_filename           = NULL;
  state->audio_device_name             = SIO_DEVANY;

  state->ctmFromFarEndDetected         = false;
  state->ctmCharacterTransmitted       = false;
  state->enquiryFromFarEndDetected     = false;
  state->syncOnBaudot                  = true;
  state->ctmTransmitterIsIdle          = true;
  state->baudotEOF                     = false;
  state->ctmEOF                        = false;
  state->disableNegotiation            = false;
  state->disableBypass                 = false;
  state->earlyMutingRequired           = false;
  state->baudotAlreadyReceived         = false;
  state->actualBaudotCharDetected      = false;
  state->baudotOutTTYCodeFifoLength    = 50;

  /* initialize the audio buffers. */
  state->ctm_input_buffer = calloc(LENGTH_TONE_VEC, sizeof(Shortint));
  state->ctm_output_buffer = calloc(LENGTH_TONE_VEC, sizeof(Shortint));
  state->baudot_input_buffer = calloc(LENGTH_TONE_VEC, sizeof(Shortint));
  state->baudot_output_buffer = calloc(LENGTH_TONE_VEC, sizeof(Shortint));

  /* set the i/o modes. */
  set_modes(state, output_mode, input_mode, ctm_output_name, ctm_input_name, user_output_name, user_input_name);

#ifdef PCAUDIO /* only if real-time audio I/O is desired */
  state->audio_hdl                     = NULL;

  sio_initpar(&state->audio_params);
  state->audio_params.rate = 8000;
  state->audio_params.bits = 16;
  state->audio_params.bps = 2;
  state->audio_params.le = SIO_LE_NATIVE;
  state->audio_params.rchan = 1;
  state->audio_params.pchan = 1;
  state->audio_params.appbufsz = LENGTH_TONE_VEC;

  /* for over/underrun testing. */
  //state->audio_params.xrun = SIO_ERROR;

  /* if the user has specified to use an audio device, open it here */
  open_audio_devices(state);

#endif

  /* set up transmitter & receiver */
  init_baudot_tonedemod(&(state->baudot_tonedemod_state));
  init_baudot_tonemod(&(state->baudot_tonemod_state));
  init_ctm_transmitter(&(state->tx_state));
  init_ctm_receiver(&(state->rx_state));  

  Shortint_fifo_init(&(state->signalFifoState), SYMB_LEN+LENGTH_TONE_VEC);
  Shortint_fifo_init(&(state->baudotOutTTYCodeFifoState), state->baudotOutTTYCodeFifoLength);
  Shortint_fifo_init(&(state->ctmOutTTYCodeFifoState),  2);
  Shortint_fifo_init(&(state->ctmToBaudotFifoState),  4000);
  Shortint_fifo_init(&(state->baudotToCtmFifoState),  3);
}

void setup_poll_fds(struct pollfd *pfds)
{
  /* setup POLL structs:
   * 0 = user input (baudot or text)
   * 1 = user output (baudot or text)
   * 2 = ctm input OR ctm audio
   * 3 = ctm output
   */

  if (state->ctm_audio_dev_mode)
    nfds = 3;
  else
    nfds = 4;

  pfds = calloc(nfds, sizeof(struct pollfd));

  pfds[0]->fd = state->userInputFileFp;
  pfds[1]->fd = state->userOutputFileFp;
  pfds[0]->events = POLLIN;
  pfds[1]->events = POLLIN;

  if (state->ctm_audio_dev_mode) {
    if (sio_pollfd(state->audio_hdl, pfds[2], POLLIN|POLLOUT) != 1)
      errx(1, "unable to setup audio device polling.");
  }
  else {
    pfds[2]->fd = state->ctmInputFileFp;
    pfds[3]->fd = state->ctmOutputfileFp;
    pfds[2]->events = POLLIN;
    pfds[3]->events = POLLIN;
  }

}
int
ctm_start(void)
{
  struct pollfd *pfds;
  int nfds;
  int r_nfds;

#ifdef PCAUDIO
  if (state->ctm_audio_dev_mode)
  {
    fprintf(stderr, "starting audio device \"%s\"...\n", state->audio_device_name);

    if(sio_start(state->audio_hdl) == 0)
      errx(1, "unable to start audio device \"%s\".\n", state->audio_device_name);

    if(sio_setvol(state->audio_hdl, SIO_MAXVOL) == 0)
      errx(1, "unable to set audio volume on device \"%s\".\n", state->audio_device_name);
  } 
#endif

  if (disableNegotiation)
    ctmFromFarEndDetected = true;

  /*
   * Main processing loop
   */
  for(;;) {
    r_nfds = poll(pfds, nfds, INFTIM);

    if (r_nfds == -1)
      err(1, NULL);
    
    for (index=0; index < nfds; index++) {
      
      switch (index) {
        case 0:
          if (pfds[index]->r_events & (POLLIN))
            layer2_process_user_input(state);
          break;
        case 1:
          if (pfds[index]->r_events & (POLLOUT))
            layer2_process_user_output(state);
          break;
        case 2:
          if (state->ctm_audio_dev_mode) {
            if(sio_revents(state->audio_hdl, pfds[2]) & (POLLIN)) {
              /* process audio in */
              layer2_process_ctm_audio_in(state);
            }
            if(sio_revents(state->audio_hdl, pfds[2]) & (POLLOUT)) {
              layer2_process_ctm_audio_out(state);
            }
          }
          else
            layer2_process_ctm_file_input(state);
          break;
        case 3:
          if (pfds[index]->r_events & (POLLIN|POLLOUT))
            layer2_process_ctm_file_output(state);
          break;
        default:
          errx(1, "invalid pollfd index.");
          break;
      }
    }
  }

  return 0;
}
