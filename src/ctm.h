#ifndef ctm_h
#define ctm_h

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
      
    Shortint     *baudot_output_buffer;
    Shortint     *ctm_output_buffer;

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

    /* Define file variables */
    
    FILE  *ctmInputFileFp;
    FILE  *baudotInputFileFp;
    FILE  *ctmOutputFileFp;
    FILE  *baudotOutputFileFp;
    FILE  *textOutputFileFp;
    FILE  *text_input_file_fp;
  
    const char* ctmInputFileName;
    const char* baudotInputFileName;
    const char* ctmOutputFileName;
    const char* baudotOutputFileName;
    const char* textOutputFileName;
    const char* text_input_filename;
    const char* audio_device_name;
  
#ifdef PCAUDIO /* only if real-time audio I/O is desired */
    struct sio_hdl *audio_hdl;
    struct sio_par audio_params; 
#endif
};
