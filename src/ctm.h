#ifndef ctm_h
#define ctm_h

#ifdef __OpenBSD__
#include <sys/types.h>
#include <sndio.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#include "typedefs.h"
#include "ctm_transmitter.h"
#include "ctm_receiver.h"
#include "baudot_functions.h"

struct ctm_state {
    Shortint     numCTMBitsStillToModulate;
    Shortint     numBaudotBitsStillToModulate;
    Shortint     cntFramesSinceBurstInit;
    Shortint     cntFramesSinceLastBypassFromCTM;
    Shortint     cntFramesSinceEnquiryDetected;
    Shortint     cntTransmittedEnquiries;
    Shortint     cntHangoverFramesForMuteBaudot;
    Shortint     ttyCode;
    UShortint    ucsCode;
  
    ULongint     cntProcessedSamples;
    ULongint     numSamplesToProcess;
  
    char         character;
      
    Bool         sineOutput;
    Bool         writeToTextFile;
    Bool         read_from_text_file;
  
    Bool         ctmReadFromFile;
    Bool         baudotReadFromFile;
    Bool         ctmWriteToFile;
    Bool         baudotWriteToFile;
    
    Bool         ctmFromFarEndDetected;
    Bool         ctmCharacterTransmitted;
    Bool         enquiryFromFarEndDetected;
    Bool         syncOnBaudot;
    Bool         ctmTransmitterIsIdle;
    Bool         baudotEOF;
    Bool         ctmEOF;
    Bool         disableNegotiation;
    Bool         disableBypass;
    Bool         earlyMutingRequired;
    Bool         baudotAlreadyReceived;
    Bool         actualBaudotCharDetected;

    Bool         compat_mode;
    Bool         ctm_audio_dev_mode; /* by default, we will use the "default" system audio device for CTM I/O. */
    Bool         shutdown_on_eof;
  
    tx_state_t   tx_state;
    rx_state_t   rx_state;
  
    /* State variables for the Baudot demodulator and modulator,respectively. */
  
    baudot_tonedemod_state_t baudot_tonedemod_state;
    baudot_tonemod_state_t   baudot_tonemod_state;

    /* Define fifo state variables */
  
    Shortint      baudotOutTTYCodeFifoLength;
    fifo_state_t  baudotOutTTYCodeFifoState;
    fifo_state_t  signalFifoState;
    fifo_state_t  ctmOutTTYCodeFifoState;
    fifo_state_t  baudotToCtmFifoState;
    fifo_state_t  ctmToBaudotFifoState;

    Shortint   *baudot_input_buffer;
    Shortint   *ctm_input_buffer;
    Shortint   *baudot_output_buffer;
    Shortint   *ctm_output_buffer;

    int        audio_buffer_size;

    /* Define file variables */
    
    int ctmInputFileFp;
    int ctmOutputFileFp;
    int userInputFileFp;
    int userOutputFileFp;
  
    const char* ctmInputFileName;
    const char* baudotInputFileName;
    const char* ctmOutputFileName;
    const char* baudotOutputFileName;
    const char* textOutputFileName;
    const char* text_input_filename;
    const char* audio_device_name;
  
    struct sio_hdl *audio_hdl;
    struct sio_par audio_params; 
};

/*
 * Enumerated types
 */

enum ctm_output_mode {
  CTM_AUDIO,
  CTM_FILE,
  CTM_FILE_COMPAT
};

enum ctm_user_input_mode {
  CTM_BAUDOT_IN,
  CTM_BAUDOT_IN_COMPAT,
  CTM_TEXT_IN
};

enum io_flag {
  CTM_IO_IN,
  CTM_IO_OUT
};

enum on_off {
  ON,
  OFF
};

/* 
 * API functions
*/

/* The first char* name is used for audio devices, if that is the mode enabled. */
void ctm_init(enum ctm_output_mode output_mode, enum ctm_user_input_mode input_mode, int, int, int, int, char *);
void ctm_set_negotiation(enum on_off);
int ctm_start(void);
void ctm_set_num_samples(int);
void ctm_set_shutdown_on_eof(int);

#endif
