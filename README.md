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

By default, ctm will use the default audio device to transmit and receive CTM signals. This behavior can be changed by using the "-f" option.

If CTM input and/or output file are specified, the sound device will not be used, and all CTM transmissions and receptions will go through the specified files. Note that these files can be stdin/stdout, audio devices, FIFOs (see mkfifo(1)), /dev/null, etc.

usage: ctm [-cbn]\n\t[-i file] [-o file] [-I file] [-O file] [-f device] [-N number]

Text mode:
                     +------------+                
    Input text  ---> |            | ---> CTM signal
                     | adaptation |                
    Output text <--- |            | <--- CTM signal
                     +------------+                

Baudot mode:
                     +------------+                
  Baudot Tones  ---> |            | ---> CTM signal
                     | adaptation |                
  Baudot Tones  <--- |            | <--- CTM signal
                     +------------+                

use: %s [ arguments ]
  -I [input_file]  input file with CTM signal (optional)
  -O [output_file] output file for CTM signal (optional)
  -i [input_file]  input file with text or baudot tones (optional)
  -o [output_file] output file for text or baudot tones (optional)
  -N [number]      number of samples to process (optional)
  -n               disables enquiry negotiation (optional)
  -c               enables compatibility mode with 3GPP test files (optional)
  -f [device]      audio device to use for CTM signals (optional)

Examples
===

1> ctm

Reads from stdin and writes to stdout. The CTM modem communicates via the default audio device.

2> ctm -n -i - -o -

The same as running 1, but does not do ENQUIRY handshaking.

3> ctm -i file1.txt -o file2.txt -f snd/0

Use "snd/0" as the CTM modem audio communication device, reading text input from file1.txt and writing text output to file2.txt.
