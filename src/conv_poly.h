/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : conv_poly.h
*      Purpose          : Header file for conv_poly.c
*      Author           : Francisco Javier Gil Gomez
*
*******************************************************************************
*/

#ifndef conv_poly_h
#define conv_poly_h "$Id: $"

#include "ctm_defines.h"   /* CHC_RATE, CHC_K */

/* The constants CHC_RATE (rate of the channel codec) and */
/* CHC_K (constraint length) are defined in ctm_defines   */

#define NUM_NODES (1<<(CHC_K-1)) /* MUST BE 2^(CHC_K-1)            */
#define BLOCK 5                  /* We perform the analysis of the */
//                               /* paths along BLOCK*CHC_K steps  */


/* POLYXYZ  =  Polynomial for generating an optimum short constraint   */
/* length convolutional code with rate = 1/X and constraint length = Y */

/* Polynoms for rate = 1/2 */

#define POLY23A 0x7    
#define POLY23B 0x5    

#define POLY24A 0XF
#define POLY24B 0XB

#define POLY25A 0X17
#define POLY25B 0X19

#define POLY26A 0X2F   
#define POLY26B 0X35

#define POLY27A 0X4F
#define POLY27B 0X6D

#define POLY28A 0X9F
#define POLY28B 0XE5

#define POLY29A 0X1AF
#define POLY29B 0X11D

/* Polynoms for rate = 1/3 */

#define POLY33A 0x7
#define POLY33B 0x7
#define POLY33C 0x5

#define POLY34A 0xF
#define POLY34B 0xB
#define POLY34C 0xD

#define POLY35A 0x1F
#define POLY35B 0x1B
#define POLY35C 0x15

#define POLY36A 0x2F
#define POLY36B 0x2B
#define POLY36C 0x3D

#define POLY37A 0x4F
#define POLY37B 0x57
#define POLY37C 0x6D

#define POLY38A 0xEF
#define POLY38B 0x9B
#define POLY38C 0xA9

/* Polynoms for rate = 1/4 */
/* cf. Lin and Costello,
   "Error Control Coding: Fundamentals and Applications" */

#define POLY43A 0x5
#define POLY43B 0x7
#define POLY43C 0x7
#define POLY43D 0x7

#define POLY44A 0xB
#define POLY44B 0xD
#define POLY44C 0xD
#define POLY44D 0xF

#define POLY45A 0x15
#define POLY45B 0x17
#define POLY45C 0x1B
#define POLY45D 0x1F

#define POLY46A 0x2B
#define POLY46B 0x37
#define POLY46C 0x39
#define POLY46D 0x3D

#define POLY47A 0x5D
#define POLY47B 0x5D
#define POLY47C 0x67
#define POLY47D 0x73

#define POLY48A 0x9D
#define POLY48B 0xBD
#define POLY48C 0xCB
#define POLY48D 0xEF

#define POLY49A 0x133
#define POLY49B 0x15D
#define POLY49C 0x1DB
#define POLY49D 0x1E5

typedef struct
{
  Shortint impulse_response[CHC_RATE*CHC_K];
  Shortint temp[CHC_RATE*CHC_K]; 
} conv_encoder_t;

typedef struct
{
  Longint metric;                 /* Metric of the node after updating      */
  Longint oldmetric;              /* Metric of the node before updating     */
  Shortint base_output;           /* Output of the node when the input is 0 */
  Shortint continue_path_from;    /* Last node from which the actual node
                                     will continue the path                 */
  Shortint new_entry;             /* Value to be added to the path (0 or 1) */
  Shortint path[BLOCK*CHC_K];     /* Path ending in the node                */
  Shortint temppath[BLOCK*CHC_K]; /* Temp path used for the updating        */
  
} node_t;

typedef struct
{
  node_t nodes[NUM_NODES];
  Shortint number_of_steps;
} viterbi_t;

void polynomials(Shortint rate, Shortint k,
                 Shortint* polya, Shortint* polyb,
                 Shortint* polyc, Shortint* polyd);

#endif
