#!/bin/sh

if ! [ -a "return_fifo" ]
then
  mkfifo return_fifo
fi

nc -lk 12345 0<return_fifo | ../openbsd/ctm -i - -o return_fifo

rm return_fifo

# in another terminal, open a connection via 'nc localhost 12345'
