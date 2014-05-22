#include <poll.h>

#include "ctm.h"

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

#include "ctm_defines.h"
#include "ctm_transmitter.h"
#include "ctm_receiver.h"
#include "baudot_functions.h"
#include "ucs_functions.h"
#include <typedefs.h>
#include <fifo.h>

/* external functions */
extern void layer2_process_user_input(struct ctm_state *);
extern void layer2_process_user_output(struct ctm_state *);
extern void layer2_process_ctm_audio_in(struct ctm_state *);
extern void layer2_process_ctm_audio_out(struct ctm_state *);
extern void layer2_process_ctm_file_input(struct ctm_state *);
extern void layer2_process_ctm_file_output(struct ctm_state *);

/* function prototypes */
static void set_modes(enum ctm_output_mode, enum ctm_user_input_mode, int, int, int, int, char *);
void open_audio_devices(void);
void ctm_set_negotiation(enum on_off);
void ctm_init(enum ctm_output_mode, enum ctm_user_input_mode, int, int, int, int, char *);
static int setup_poll_fds(struct pollfd *, int);
int ctm_start(void);
void ctm_set_num_samples(int);
void ctm_set_shutdown_on_eof(int);

static struct ctm_state *state;

void ctm_set_num_samples(int num_samples)
{
  state->numSamplesToProcess = num_samples;
}

void ctm_set_shutdown_on_eof(int flag)
{
  if(flag == 1)
    state->shutdown_on_eof = true;
  else
    state->shutdown_on_eof = false;
}

static void set_modes(enum ctm_output_mode ctm_output_mode, enum ctm_user_input_mode input_mode, int ctm_output_fd, int ctm_input_fd, int user_output_fd, int user_input_fd, char *device_name)
{
  switch(ctm_output_mode) {
    case CTM_AUDIO:
      /* this is the default mode. */
      state->audio_device_name = device_name; 
      break;
    case CTM_FILE:
      state->ctmReadFromFile           = true;
      state->ctmWriteToFile            = true;
      state->ctm_audio_dev_mode        = false;

      state->ctmInputFileFp = ctm_input_fd;
      state->ctmOutputFileFp = ctm_output_fd;
      break;
    case CTM_FILE_COMPAT:
      state->ctmReadFromFile           = true;
      state->ctmWriteToFile            = true;
      state->ctm_audio_dev_mode        = false;
      state->compat_mode               = true;

      state->ctmInputFileFp = ctm_input_fd;
      state->ctmOutputFileFp = ctm_output_fd;
      break;
    default:
      errx(1, "invalid CTM mode.");
      break;
  }

  switch(input_mode) {
    case CTM_TEXT_IN:
      /* this is the default user input mode. */
      state->userInputFileFp = user_input_fd;
      state->userOutputFileFp = user_output_fd;
      break;
    case CTM_BAUDOT_IN:
      state->writeToTextFile           = false;
      state->read_from_text_file       = false;
      state->baudotReadFromFile        = true;
      state->baudotWriteToFile         = true;
      state->userInputFileFp = user_input_fd;
      state->userOutputFileFp = user_output_fd;
      break;
    case CTM_BAUDOT_IN_COMPAT:
      state->writeToTextFile           = false;
      state->read_from_text_file       = false;
      state->baudotReadFromFile        = true;
      state->baudotWriteToFile         = true;
      state->compat_mode               = true;
      state->userInputFileFp = user_input_fd;
      state->userOutputFileFp = user_output_fd;
      break;
    default:
      errx(1, "invalid user input mode.");
      break;
  }
}

void open_audio_devices(void)
{
  if (state->ctm_audio_dev_mode)
  {
    fprintf(stderr, "opening audio device for duplex i/o...\n");
    state->audio_hdl = sio_open(state->audio_device_name, SIO_PLAY | SIO_REC, 1);
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
void ctm_set_negotiation(enum on_off flag)
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

void ctm_init(enum ctm_output_mode output_mode, enum ctm_user_input_mode input_mode, int ctm_output_fd, int ctm_input_fd, int user_output_fd, int user_input_fd, char *device_name)
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
  state->ctmInputFileFp                = -1;
  state->userInputFileFp               = STDIN_FILENO;
  state->ctmOutputFileFp               = -1;
  state->userOutputFileFp              = STDOUT_FILENO;

  state->ctmInputFileName              = NULL;
  state->baudotInputFileName           = NULL;
  state->ctmOutputFileName             = NULL;
  state->baudotOutputFileName          = NULL;
  state->textOutputFileName            = NULL;
  state->text_input_filename           = NULL;
  state->audio_device_name             = device_name;

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

  state->audio_buffer_size             = LENGTH_TONE_VEC * sizeof(Shortint);

  /* initialize the audio buffers. */
  state->ctm_input_buffer = calloc(LENGTH_TONE_VEC, sizeof(Shortint));
  state->ctm_output_buffer = calloc(LENGTH_TONE_VEC, sizeof(Shortint));
  state->baudot_input_buffer = calloc(LENGTH_TONE_VEC, sizeof(Shortint));
  state->baudot_output_buffer = calloc(LENGTH_TONE_VEC, sizeof(Shortint));

  /* set the i/o modes. */
  set_modes(output_mode, input_mode, ctm_output_fd, ctm_input_fd, user_output_fd, user_input_fd, device_name);

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
  open_audio_devices();

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

static int setup_poll_fds(struct pollfd *pfds, int nfds)
{
  /* setup POLL structs:
   * 0 = user input (baudot or text)
   * 1 = ctm input OR ctm audio
   */

  int active_nfds = nfds;

  if (nfds > 0)
    pfds[0].fd = state->userInputFileFp;

  /* if we have already hit an EOF condition on the input, stop polling. 
   * This avoids high cpu usage.
   * */
  if (!state->baudotEOF)
    pfds[0].events = POLLIN;
  else
  {
    pfds[0].events = 0;
    active_nfds -= 1;
  }

  if (state->ctmReadFromFile && nfds > 1)
  {
    if(state->ctmEOF) {
      /* stop polling at CTM EOF. */
      pfds[1].events = 0;
      active_nfds -= 1;
    }
    else
    {
      pfds[1].events = POLLIN;
      pfds[1].fd = state->ctmInputFileFp;
    }
  }

  if (state->ctm_audio_dev_mode && nfds > 1) {
    if (sio_pollfd(state->audio_hdl, &pfds[1], POLLIN|POLLOUT) != 1)
      errx(1, "unable to setup audio device polling.");
  }

  return active_nfds;
}

int ctm_start(void)
{
  struct pollfd *pfds = NULL;
  int nfds;
  int r_nfds;
  int index;
  int active_nfds;

  nfds = 2;

  if ((pfds = calloc(nfds, sizeof(struct pollfd))) == NULL)
    err(1, "ctm_start: pfds == NULL");

  if (state->ctm_audio_dev_mode)
  {
    fprintf(stderr, "starting audio device \"%s\"...\n", state->audio_device_name);

    if(sio_start(state->audio_hdl) == 0)
      errx(1, "unable to start audio device \"%s\".\n", state->audio_device_name);

    if(sio_setvol(state->audio_hdl, SIO_MAXVOL) == 0)
      errx(1, "unable to set audio volume on device \"%s\".\n", state->audio_device_name);
  } 

  if (state->disableNegotiation)
    state->ctmFromFarEndDetected = true;

  /*
   * Main processing loop
   */
  for(;;) {
    active_nfds = setup_poll_fds(pfds, nfds);

    if (active_nfds > 0)
    {
      r_nfds = poll(pfds, nfds, INFTIM);

      if (r_nfds == -1)
        err(1, "ctm_start: polling error");

      for (index=0; index < nfds; index++) {

        switch (index) {
          case 0:
            if ((pfds[index].revents & POLLIN) == POLLIN)
              layer2_process_user_input(state);
            break;
          case 1:
            if (state->ctm_audio_dev_mode) {
              if((sio_revents(state->audio_hdl, &pfds[index]) & POLLIN) == POLLIN) {
                layer2_process_ctm_audio_in(state);
              }
              if((sio_revents(state->audio_hdl, &pfds[index]) & POLLOUT) == POLLOUT) {
                layer2_process_ctm_audio_out(state);
              }
            }
            else
              if ((pfds[index].revents & POLLIN) == POLLIN)
                layer2_process_ctm_file_input(state);
            break;
          default:
            errx(1, "ctm_start: invalid pollfd index.");
            break;
        }
      }
    }

    /* process output files here, as these never block. */
    layer2_process_user_output(state);

    if (!state->ctm_audio_dev_mode)
      layer2_process_ctm_file_output(state);

    /* conditions to break the loop */
    if ((state->numSamplesToProcess > 0 && state->numSamplesToProcess <= state->cntProcessedSamples) ||
        (state->baudotEOF && state->ctmEOF && state->ctmTransmitterIsIdle && (Shortint_fifo_check(&(state->ctmToBaudotFifoState)) == 0) &&
         (state->numBaudotBitsStillToModulate == 0)))
      break;
    /* break on user text input EOF, if desired. */
    if (state->shutdown_on_eof && state->baudotEOF && state->ctmTransmitterIsIdle && (Shortint_fifo_check(&(state->ctmToBaudotFifoState)) == 0))
      break;
  }

  return 0;
}
