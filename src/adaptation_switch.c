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
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <termios.h>

#ifdef __OpenBSD__
#include <sys/types.h>
#include <sys/stat.h>
#include <sndio.h>
#define PCAUDIO
#endif

/***********************************************************************/

void usage()
{
  fprintf(stderr, "usage: ctm [-cbn]\n\t[-i file] [-o file] [-I file]\n\t[-O file] [-f device] [-N number]\n");
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
    else
      file_fd = STDIN_FILENO;
  }

  else
  {
    if ((file_fd = open(filename, flags, 0666)) == -1)
      errx(1, "unable to open %s", filename);
  }

  return file_fd;
}

/***********************************************************************/

int main(int argc, char** argv)
{
  Shortint     cnt;
  int          optindex;
  const char*  errstr;

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
  int shutdown_on_eof_flag;

  enum ctm_user_input_mode user_input_mode;
  enum ctm_output_mode ctm_mode;
  int num_samples;

  /* set default behavior */
  user_input_fd = STDIN_FILENO;
  user_output_fd = STDOUT_FILENO;
  ctm_input_fd = -1;
  ctm_output_fd = -1;
  compat_flag = 0;
  baudot_flag = 0;
  negotiation_flag = 0;
  ctm_file_mode_flag = 0;
  audio_mode_flag = 1;
  num_samples = -1; /* by default, set to infinite */
  shutdown_on_eof_flag = 0;

  int ch;
  while ((ch = getopt(argc, argv, "scbni:o:f:I:O:N:")) != -1) {
    switch (ch) {
      case 's':
        shutdown_on_eof_flag = 1;
        break;
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
        ctm_input_fd = open_file_or_stdio(optarg, O_RDONLY | O_NONBLOCK);
        break;
      case 'O':
        ctm_file_mode_flag = 1;
        audio_mode_flag = 0;
        ctm_output_fd = open_file_or_stdio(optarg, O_WRONLY | O_NONBLOCK | O_CREAT | O_TRUNC);
        break;
      case 'i':
        user_input_fd = open_file_or_stdio(optarg, O_RDONLY | O_NONBLOCK);
        break;
      case 'o':
        user_output_fd = open_file_or_stdio(optarg, O_WRONLY | O_NONBLOCK | O_CREAT | O_TRUNC);
        break;
      case 'N':
        num_samples = strtonum(optarg, 1, ULONG_MAX, &errstr); 
        if (errstr)
          errx(1, "number of samples is %s: %s", errstr, optarg);
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

  else if (ctm_file_mode_flag == 1 && (ctm_input_fd == -1 && ctm_output_fd == -1))
    errx(1, "invalid arguments: both CTM input and output files must be specified.");

  else if (compat_flag == 1 && ctm_file_mode_flag == 0 && baudot_flag == 0)
    errx(1, "invalid arguments: compatibility mode is used only with baudot and/or CTM file modes.");

  /* select the user input mode and CTM mode based on the input arguments. */
  if (ctm_file_mode_flag == 1) {
    if (compat_flag == 0)
      ctm_mode = CTM_FILE;
    else
      ctm_mode = CTM_FILE_COMPAT;
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

  ctm_init(ctm_mode, user_input_mode, ctm_output_fd, ctm_input_fd, user_output_fd, user_input_fd, SIO_DEVANY);
  ctm_set_negotiation(negotiation_flag);
  ctm_set_shutdown_on_eof(shutdown_on_eof_flag);
  ctm_set_num_samples(num_samples);
  ctm_start();

  /* if in audio mode, this will never return. User must signal process to stop. */

  exit(0);
}
