###############################################################################
#   
#       # 
###############################################################################
# 
#       File             : Makefile
#       Author           : EED/N/RV Matthias Doerbecker
#       Tested Platforms : SUN Solaris, 
#                          Gnu C Compiler (gcc version 2.7.2.3),
#                          GNU Make version 3.77
#
#       Description      : Platform-independent Makefile;
#                          all object and executable files are written to
#                          the directory ./$(OSTYPE)
#
#                          Use:
#                              make clean    -- delete all object files
# 
#                              make
#                          or
#                              make all      -- compile all files in module
#                                               and build example program
#
#                          Depending on your local installation, it might be
#                          necessary to use "gmake" instead of "make"
# 
#       Doc Id           : $Id: $
# 
###############################################################################

OSTYPE = solaris
MODE=NORM

#
#
# use the GNU compiler
#
CC          = gcc

#
# select CC flags according to mode
#

CFLAGS = -O6 -Wall  -I.

#
# linker flags
#
LDFLAGS  = 	-lm -lsndio $(LLDFLAGS)   


#
# all local module source/include/object files
#
MODULE_SOURCES  = diag_deinterleaver.c diag_interleaver.c \
                  init_interleaver.c m_sequence.c \
                  conv_encoder.c viterbi.c conv_poly.c \
                  tonedemod.c tonemod.c wait_for_sync.c \
                  baudot_functions.c ucs_functions.c \
                  ctm_receiver.c ctm_transmitter.c \
                  sin_fip.c fifo.c parse_arg.c


MODULE_INCLUDES = $(MODULE_SOURCES:.c=.h)
MODULE_OBJECTS  = $(patsubst %,$(OSTYPE)/%,$(MODULE_SOURCES:.c=.o))

#
# files needed for the main program(s)
#
MAIN_SOURCES = adaptation_switch.c 
MAIN_OBJECTS = $(patsubst %,$(OSTYPE)/%,$(MAIN_SOURCES:.c=.o))

VPATH = ./$(OSTYPE)

#
# default target
#
all: $(patsubst %,$(OSTYPE)/%,$(MAIN_SOURCES:.c=))

#
# clean up: delete object files
#
clean:
	rm -f  $(OSTYPE)/*.o  

#
# implicit rule about how to compile a C program
#
.SUFFIXES:         # Delete the default suffixes
.SUFFIXES: .c .o   # Define our suffix list
.c.o:
	(CC) -c $(CFLAGS) -o $@ $<


#
# rules how to bulid the .o files
#

$(OSTYPE)/%.o: %.c ctm_defines.h init_interleaver.h  Makefile  $(OSTYPE)
	$(CC) -c $(CFLAGS) -o $@ $<

#
# rules how to bulid the main program(s)
#

$(OSTYPE)/adaptation_switch: $(OSTYPE)/adaptation_switch.o $(DSPMODULES_OBJ) $(AUIDOMODULES_OBJ) $(MODULE_OBJECTS)  Makefile  $(OSTYPE)
	$(CC) -o $@  $(CFLAGS)  $< $(MODULE_OBJECTS)  $(LDFLAGS)

# rules how to make platform-dependent target directory
#
$(OSTYPE):
	mkdir $(OSTYPE)



