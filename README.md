ctm
===

Cellular Text Modem, based on the 3GPP reference implementation, release 11.0.

This program allows you to communicate data over a cellular voice channel. The user data rate is 45.5 bps, which is the standard baudot TTY rate for U.S. services in ITU V.18.

History
===

The original software can be found here:

http://www.3gpp.org/DynaReport/26230.htm

From the 3GPP release information, the source appears unchanged since Rel-6, dated 2004-12-16. Copyright information was removed in Rel-5.

Usage
===

The ctm program has two primary modes of operation: text input and baudot input. Text input allows the user to specify input/output text files (or stdin/stdout). Baudot mode takes as input baudot PCM files.

Called with no arguments, ctm will read text input from stdin, write text output to stdout, and use the default audio device on the system for CTM communications.

By default, ctm will use the default audio device to transmit and receive CTM signals. This behavior can be changed by using the "-ctmdevice" option.

If CTM input and/or output file are specified, the sound device will not be used, and all CTM transmissions and receptions will go through the specified files. Note that these files can be stdin/stdout, audio devices, FIFOs (see mkfifo(1)), /dev/null, etc.

                     +------------+                
    Input text  ---> |            | ---> CTM signal
                     | adaptation |                
    Output text <--- |            | <--- CTM signal
                     +------------+                

                     +------------+                
  Baudot Tones  ---> |            | ---> CTM signal
                     | adaptation |                
  Baudot Tones  <--- |            | <--- CTM signal
                     +------------+                

use: %s [ arguments ]
  -ctmin      <input_file>   input file with CTM signal (optional)
  -ctmout     <output_file>  output file for CTM signal (optional)
  -baudotin   <input_file>   input file with Baudot Tones (optional)
  -baudotout  <output_file>  output file for Baudot Tones (optional)
  -textout    <text_file>    output text file from CTM receiver (optional)
  -textin     <text_file>    input text file to CTM transmitter (optional)
  -numsamples <number>       number of samples to process (optional)
  -nonegotiation             disables the negotiation (optional)
  -nobypass                  disables the signal bypass (optional)
  -compat                     enables compatibility mode with 3GPP test files (optional)
  -ctmdevice <device_name>   audio device to use for CTM signals (optional)
