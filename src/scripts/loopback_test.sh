#!/bin/sh

if ! [ -a "loopback_fifo" ]
then
  mkfifo loopback_fifo
fi

if [ -a "test_output.txt" ]
then
  rm test_output.txt
fi

../openbsd/ctm -s -i test_input.txt -o test_output.txt -I loopback_fifo -O loopback_fifo 2>/dev/null

if diff test_input.txt test_output.txt 
then
  echo "loopback test PASSED"
else
  echo "loopback test FAILED"
fi

rm loopback_fifo
rm test_output.txt
