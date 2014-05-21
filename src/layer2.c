#include "ctm.h"

void layer2_process_user_input(struct ctm_state *state)
{
  if (state->baudotReadFromFile)
  {
    /* if the baudot out FIFO isn't already full, grab more samples. */
    if (Shortint_fifo_check(&(state->baudotOutTTYCodeFifoState)) < state->baudotOutTTYCodeLength) {
      if(read(state->userInputFileFp, state->baudot_input_buffer, state->audio_buffer_size) < state->audio_buffer_size)
      {
        /* if EOF is reached, use buffer with zeros instead */
        baudotEOF = true;
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
  }

  else {
    /* otherwise we are reading text input */
    if (Shortint_fifo_check(&(state->baudotToCtmFifoState)) < state->baudotToCtmFifoStateLength)
    {
      if (read(state->userInputFileFp, &(state->input_char), 1) == -1)
        err(1, NULL);

      state->ucsCode = convertChar2UCScode(state->input_char);
      Shortint_fifo_push(&(state->baudotToCtmFifoState), &(state->ucsCode), 1);
    }
  }
} 

void layer2_process_user_output(struct ctm_state *state)
{
  if (state->baudotReadFromFile) {
  }

  else {
    /* otherwise we are writing to a text file */
    if (write(state->userOutputFileFp, &character, 1) == -1)
      errx(1, NULL);
  }
}

void layer2_process_ctm_audio_in(struct ctm_state *state)
{
  if (sio_read(state->audio_hdl, state->ctm_input_buffer, state->audio_buffer_size) < audio_buffer_size) {
    warnx(1, "underrun in audio input from device.");
  }
}

void layer2_process_ctm_audio_out(struct ctm_state *state)
{
  if (sio_write(state->audio_hdl, state->ctm_input_buffer, state->audio_buffer_size) < audio_buffer_size) {
    warnx(1, "overrun in audio output to device.");
  }
}

void layer2_process_ctm_file_input(struct ctm_state *state)
{
}

void layer2_process_ctm_file_output(struct ctm_start *state)
{
}
