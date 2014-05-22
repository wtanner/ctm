#!/bin/sh

if ! [ -a "loopback_fifo" ]
then
  mkfifo loopback_fifo
fi

../openbsd/ctm -s -i test_input.txt -o test_output.txt -I loopback_fifo -O loopback_fifo 2>/dev/null
rm loopback_fifo

# in another terminal, open a connection via 'nc localhost 12345'
