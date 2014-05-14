/*
*******************************************************************************
*
*      *******************************************************************************
*
*      File             : init_interleaver.h
*      Purpose          : initialization of the diagonal (chain) interleaver;
*                         definition of the type interleaver_state_t
*
*******************************************************************************
*/
#ifndef init_interleaver_h
#define init_interleaver_h "$Id: $"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include "typedefs.h"

/*
*******************************************************************************
*                         DECLARATION OF PROTOTYPES
*******************************************************************************
*/

/* --------------------------------------------------------------------- */
/* interleaver_state_t:                                                  */
/* type definition for storing the states of the diag_interleaver and    */
/* diag_deinterleaver, respectively (each instance of                    */
/* interleavers/deinterleavers must have its own variable of this type   */ 
/* --------------------------------------------------------------------- */

typedef struct {
  Shortint B, D;           /* dimensions of the (de)interleaver              */
  Shortint rows;           /* Number of rows in buffer                       */
  Shortint clmn;           /* actual index within the (de)interleaver matrix */
  Shortint ready;          /* Number of ready rows in (de)interleaver        */
  Shortint *vector;        /* memory of the (de)interleaver                  */
  Shortint num_sync_lines1;/* number of preceding lines in the interl. matrix*/
  Shortint num_sync_lines2;/* number of preceding lines in the interl. matrix*/
  Shortint num_sync_bits;  /* number of sync bits (demodulator sync)         */
  Shortint *sync_index_vec;/* indices of the bits for deintl. synchronization*/
  Shortint *scramble_vec;  /* sequence for scrambling                        */
  Shortint *sequence;      /* m-sequence for synchronisation                 */
  
} interleaver_state_t;


/* --------------------------------------------------------------------- */
/* init_interleaver:                                                     */
/* function for initialization of diag_interleaver and                   */
/* diag_deinterleaver, respectively. The dimensions of the interleaver   */
/* must be specified:                                                    */
/* B = (horizontal) blocklength, D = (vertical distance)                 */
/* According to this specifications, this function initializes a         */
/* variable of type interleaver_state_t.                                 */
/*                                                                       */
/* Additionally, this function adds two types of sync information to the */
/* bitstream. The first sync info is for the demodulator and consists    */
/* of a sequence of alternating bits so that the tones produced by the   */
/* modulator are not the same all the time. This is essential for the    */
/* demodulator to find the transitions between adjacent bits. The bits   */
/* for this demodulator synchronization simply precede the bitstream.    */
/* A good choice for this is e.g. num_sync_lines1=2, which results in    */
/* 14 additional bits in case of the (B=7, D=2) interleaver.             */
/*                                                                       */
/* The second sync info is for synchronizing the deinterleaver and       */
/* of a m-sequence with excellent autocorrelation properties. These bits */
/* are positioned at the locations of the dummy bits, which are not used */
/* by the interleaver. In addition, even more bits for this              */
/* can be spent by inserting additional sync bits, which precede the     */
/* interleaver's bitstream. This is indicated by chosing                 */
/* num_sync_lines2>0.                                                    */
/*                                                                       */
/* Example: (B=7, D=2)-interleaver,                                      */
/*          num_sync_lines1=2 (demodulator sync bits are marked by 'd')  */
/*          num_sync_lines2=1 (deinterleaver sync bits are marked by 'x')*/
/*                                                                       */
/*     d     d     d     d     d     d     d                             */
/*     d     d     d     d     d     d     d                             */
/*     x     x     x     x     x     x     x                             */
/*     1     x     x     x     x     x     x                             */
/*     8     x     x     x     x     x     x                             */
/*    15     2     x     x     x     x     x                             */
/*    22     9     x     x     x     x     x                             */
/*    29    16     3     x     x     x     x                             */
/*    36    23    10     x     x     x     x                             */
/*    43    30    17     4     x     x     x                             */
/*    50    37    24    11     x     x     x                             */
/*    57    44    31    18     5     x     x                             */
/*    64    51    38    25    12     x     x                             */
/*    71    58    45    32    19     6     x                             */
/*    78    65    52    39    26    13     x                             */
/*    85    72    59    46    33    20     7                             */
/*    92    79    66    53    40    27    14                             */
/*    99    86    73    60    47    34    21                             */
/* --------------------------------------------------------------------- */

void init_interleaver(interleaver_state_t *intl_state, 
                      Shortint B, Shortint D,
                      Shortint num_sync_lines1, Shortint num_sync_lines2);

/* --------------------------------------------------------------------- */
/* reinit_interleaver:                                                   */
/* Same as init_interleaver() but without new allocation of buffers.     */
/* This function shall be used for initiation each new burst.            */
/* --------------------------------------------------------------------- */

void reinit_interleaver(interleaver_state_t *intl_state);


void init_deinterleaver(interleaver_state_t *intl_state, 
                        Shortint B, Shortint D);

void reinit_deinterleaver(interleaver_state_t *intl_state);

/* --------------------------------------------------------------------- */
/* calc_mute_positions:                                                  */
/* Calculation of the indices of the bits that have to be muted within   */
/* one burst. The indices are returned in the vector mute_positions.     */
/* --------------------------------------------------------------------- */

void calc_mute_positions(Shortint *mute_positions, 
                         Shortint num_rows_to_mute,
                         Shortint start_position,
                         Shortint B, 
                         Shortint D);

Bool mutingRequired(Shortint  actualIndex, 
                    Shortint *mute_positions, 
                    Shortint  length_mute_positions);

/* --------------------------------------------------------------------- */
/* generate_scrambling_sequence:                                         */
/* Generation of the sequence used for scrambling. The sequence consists */
/* of 0 and 1 elements. The sequence is stored into the vector *sequence */
/* and the length of the sequence is specified by the variable length.   */
/* --------------------------------------------------------------------- */

void generate_scrambling_sequence(Shortint *sequence, Shortint length);

#endif



