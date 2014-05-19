/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File          : ctm_defines.h
*      Purpose       : Global constants for the Cellular Text Telephone Modem
*
*******************************************************************************
*/
#ifndef ctm_defines_h
#define ctm_defines_h "$Id: $"

#include <typedefs.h>

#ifndef MAX
#define MAX(A, B)  ((A) > (B) ? (A) : (B))
#endif

#define MAX_IDLE_SYMB        5  /* Number of Idle Symbols at End of Burst    */
#define CHC_RATE             4  /* Rate of the Error Protection              */
#define CHC_K                5  /* Constraint length of Error Protection     */
#define SYMB_LEN            40  /* Length of one CTM symbol                  */

/* The following constant determines whether the signal processing takes */
/* place sample-by-sample or frame-by-frame (160 samples per frame)      */
#define LENGTH_TONE_VEC  160  /* signal frame size                         */
//#define LENGTH_TONE_VEC      1  /* signal frame size                         */

/* Number of bits that are processed during each call of the main loop   */
#define LENGTH_TX_BITS  MAX(2, 2*LENGTH_TONE_VEC/SYMB_LEN) 

#define BITS_PER_SYMB        8  /* bits per symbol                           */

#define NCYCLES_0            2  /* Number of periods for symbol #0           */
#define NCYCLES_1            3  /* Number of periods for symbol #1           */
#define NCYCLES_2            4  /* Number of periods for symbol #2           */
#define NCYCLES_3            5  /* Number of periods for symbol #3           */

#define THRESHOLD_RELIABILITY_FOR_SUPPRESSING_OUTPUT  100
#define THRESHOLD_RELIABILITY_FOR_XCORR               200
#define THRESHOLD_RELIABILITY_FOR_GOING_OFFLINE       100
#define MAX_NUM_UNRELIABLE_GROSS_BITS                 400

#define NUM_BITS_GUARD_INTERVAL   6       /* length of silence after a burst */

#define WAIT_SYNC_REL_THRESHOLD_0 20316   /* = 0.62*32768 */
#define WAIT_SYNC_REL_THRESHOLD_1 17039   /* = 0.52*32768 */
#define WAIT_SYNC_REL_THRESHOLD_2 23265   /* = 0.71*32768 */
#define RESYNC_REL_THRESHOLD      26542   /* = 0.81*32768 */

#define GUARD_BIT_SYMBOL    10  /* "magic number" indicating that a          */
//                              /* bit shall be muted                        */

#define intlvB               8  /* Interleaver block length                  */
#define intlvD               2  /* Interleaver block distance (interlace)    */

#define demodSyncLns         1  /* Nr of demodulator sync lines              */
#define deintSyncLns         0  /* Nr of deinterleaver sync lines            */

#define IDLE_SYMB         0x16  /* UCS code for Idle Symbol                  */
#define ENQU_SYMB         0x05  /* UCS code for Enquiry Symbol               */

#if LENGTH_TONE_VEC==160
#define ENQUIRY_TIMEOUT     20  /* number of frames for negotiation          */
#else
#define ENQUIRY_TIMEOUT 19*160 
#endif

#define NUM_ENQUIRY_BURSTS   3  /* number of enquiry attempts                */

#define NUM_MUTE_ROWS        4  /* duration of muting interval, 4 rows=80ms  */
#define RESYNC_SEQ_LENGTH   32  /* must be a multiple of intlvB              */

#define NUM_BITS_BETWEEN_RESYNC  352 // 352 = 320+32
/* 352 is a multiple of CHC_RATE, intlvB, and BITS_PER_SYMB,                 */
/* and must be greater than  intlvB*((intlvB-1)*intlvD+NUM_MUTE_ROWS         */


#define PLAYBACK_VOLUME      0xC000 /* volume for sound card playback        */

//#define DEBUG_OUTPUT  /* comment this out for regular operation mode!     */

#endif
