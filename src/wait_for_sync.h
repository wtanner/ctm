/*
*******************************************************************************
*
*******************************************************************************
*
*      File             : wait_for_sync.h
*      Purpose          : synchronization routine for the deinterleaver
*
*******************************************************************************
*/
#ifndef wait_for_sync_h
#define wait_for_sync_h "$Id: $"

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

typedef struct {
  Shortint *shift_reg;         /* shift register                           */
  Shortint *m_sequence;        /* maximum length sequence (preamble)       */
  Shortint *m_sequence_resync; /* maximum length sequence (resync)         */
  Shortint *sync_index_vec;    /* positions/indices of the preamble        */
  Shortint *resync_index_vec;  /* positions/indices of the resync sequence */
  Shortint length_shift_reg;   /* length of the vector shift_reg           */
  Shortint offset;             /*                                          */
  Shortint num_sync_bits;      /* length of the preamble                   */
  Bool     sync_found;         /* true if receiver is "in sync"            */
  Bool     alreadyCTMreceived; /* true if burst has been received earlier  */
  Shortint *xcorr1_shiftreg;
  Shortint *xcorr2_shiftreg;
  UShortint cntSymbolsSinceEndOfBurst;
} wait_for_sync_state_t;



/* ----------------------------------------------------------------------- */
/* Function init_wait_for_sync()                                           */
/* *****************************                                           */
/* Initialization of the synchronization detector. The dimensions of the   */
/* corresponding interleaver at the TX side must be specified:             */
/* B                 (horizontal) blocklength                              */
/* D                 (vertical) interlace factor                           */
/* num_sync_lines2   number of interleaver lines with additional sync bits */
/* ptr_wait_state    pointer to the state variable of the sync detector    */
/* ----------------------------------------------------------------------- */

void init_wait_for_sync(wait_for_sync_state_t *ptr_wait_state,
                        Shortint B, Shortint D,
                        Shortint num_sync_lines2);


/* ----------------------------------------------------------------------- */
/* Function reinit_wait_for_sync()                                         */
/* *******************************                                         */
/* Reinitialization of synchronization detector. This function is used in  */
/* case that a burst has been finished and the transmitter has switched    */
/* into idle mode. After calling reinit_wait_for_sync(), the function      */
/* wait_for_sync() inhibits the transmission of the demodulated bits to    */
/* the deinterleaver, until the next synchronization sequence can be       */
/* detected.                                                               */
/* ----------------------------------------------------------------------- */

void reinit_wait_for_sync(wait_for_sync_state_t *ptr_wait_state);



/* ----------------------------------------------------------------------- */
/* Function wait_for_sync()                                                */
/* ************************                                                */
/* This function shall be inserted between the demodulator and the         */
/* deinterleaver. The function searches the synchronization bitstream      */
/* and cuts all received heading bits. As long as no sync is found, this   */
/* function returns *ptr_num_valid_out_bits=0 so that the main program     */
/* is able to skip the deinterleaver as long as no valid bits are          */
/* available. If the sync info is found, the complete internal shift       */
/* register is copied to out_bits so that wait_for_sync can be transparent */
/* and causes no delay for future calls.                                   */
/* *ptr_wait_interval returns a value of 0 after such a synchronization    */
/* indicating that this was a regular synchronization.                     */
/*                                                                         */
/* Regularly, the initial preamble of each burst is used as sync info.     */
/* In addition, the resynchronization sequences, which occur periodically  */
/* during a running burst, are used as "back-up" synchronization in order  */
/* to avoid loosing all characters of a burst, if the preamble was not     */
/* detected.                                                               */
/* If the receiver is already synchronized on a running burst              */
/* and the resynchronization sequence is detected, *ptr_resync_detected    */
/* returns a non-negative value in the range 0...num_in_bits-1 indicating  */
/* at which bit the resynchronization sequence has been detected. If no    */
/* resynchronization has been detected, *ptr_resync_detected is -1.        */
/* If the receiver is NOT synchronized and the resynchronization sequence  */
/* is detected, the resynchronization sequence is used as initial          */
/* synchronization. *ptr_wait_interval returns a value of 32 in this case  */
/* due to the different alignments of the synchronizations based on the    */
/* preamble or the resynchronization sequence, respectively.               */
/*                                                                         */
/* In order to carry all bits, the minimum length of the vector out_bits   */
/* must be:                                                                */
/* in_bits.size()-1 + ptr_wait_state->shift_reg_length                     */
/*                                                                         */
/* in_bits                     Vector with bits from the demodulator. The  */
/*                             vector's length can be arbitrarily chosen,  */
/*                             i.e. according to the block length of the   */
/*                             signal processing of the main program.      */
/* num_in_bits                 length of vector in_bits                    */
/* num_received_idle_symbols   number of idle symbols received coherently  */
/* out_bits                    Vector with bits for the deinterleaver.     */
/*                             The number of the valid bits is indicated   */
/*                             by *ptr_num_valid_out_bits.                 */
/* *ptr_num_valid_out_bits     returns the number of valid output bits     */
/* *ptr_wait_interval          returns either 0 or 32                      */
/* *ptr_resync_detected        returns a value -1, 0,...num_in_bits        */
/* *ptr_early_muting_required  returns whether the original audio signal   */
/*                             must not be forwarded. This is to guarantee */
/*                             that the preamble or resync sequence is     */
/*                             detected only by the first CTM device, if   */
/*                             several CTM devices are cascaded            */
/*                             subsequently.                               */
/* *ptr_wait_state             state information. This variable must be    */
/*                             initialized with init_wait_for_sync()       */
/* ----------------------------------------------------------------------- */

Bool wait_for_sync(Shortint *out_bits,
                   Shortint *in_bits,
                   Shortint  num_in_bits,
                   Shortint  num_received_idle_symbols,
                   Shortint  *ptr_num_valid_out_bits,
                   Shortint  *ptr_wait_interval,
                   Shortint  *ptr_resync_detected,
                   Bool      *ptr_early_muting_required,
                   wait_for_sync_state_t *ptr_wait_state);




/* ----------------------------------------------------------------------- */
/* Function generate_resync_sequence()                                     */
/* ***********************************                                     */
/* Generation of the sequence for resynchronization. The sequence, which   */
/* has a length according to the value of the constant RESYNC_SEQ_LENGTH,  */
/* is written to the vector *sequence, which must have been allocated      */
/* before calling this function.                                           */
/* ----------------------------------------------------------------------- */

void generate_resync_sequence(Shortint *sequence);


#endif

