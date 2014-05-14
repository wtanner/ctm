/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : conv_poly.c
*      Purpose          : Polynomials for convolutional encoder  
*                         and viterbi decoder
*      Author           : Francisco Javier Gil Gomez
*
*      Revision history
*
*      $Log: $
*
*******************************************************************************
*/

#include <stdio.h>
#include <typedefs.h>
#include "conv_poly.h"


void polynomials(Shortint rate, Shortint k,
                 Shortint* polya, Shortint* polyb,
                 Shortint* polyc, Shortint* polyd)
{
  switch (rate){
    case 2:
      *polyc = 0;
      *polyd = 0;
      switch (k){
        case 3:
          *polya = POLY23A;
          *polyb = POLY23B;
          break;
        case 4:
          *polya = POLY24A;
          *polyb = POLY24B;
          break;          
        case 5:
          *polya = POLY25A;
          *polyb = POLY25B;
          break;
        case 6:
          *polya = POLY26A;
          *polyb = POLY26B;
          break;          
        case 7:
          *polya = POLY27A;
          *polyb = POLY27B;
          break;          
        case 8:
          *polya = POLY28A;
          *polyb = POLY28B;
          break;
        case 9:
          *polya = POLY29A;
          *polyb = POLY29B;
          break;
        default: printf ("\n invalid value for K \n");
      }
      break;

    case 3:
      *polyd = 0;
      switch (k){
        case 3:
          *polya = POLY33A;
          *polyb = POLY33B;
          *polyc = POLY33C;
          break;
        case 4:
          *polya = POLY34A;
          *polyb = POLY34B;
          *polyc = POLY34C;
          break;          
        case 5:
          *polya = POLY35A;
          *polyb = POLY35B;
          *polyc = POLY35C;
          break;
        case 6:
          *polya = POLY36A;
          *polyb = POLY36B;
          *polyc = POLY36C;
          break;
        case 7:
          *polya = POLY37A;
          *polyb = POLY37B;
          *polyc = POLY37C;
          break;
        case 8:
          *polya = POLY38A;
          *polyb = POLY38B;
          *polyc = POLY38C;
          break;          
        default: printf ("\n invalid value for K \n");
      }
      break;
      
    case 4:
      switch (k){
        case 3:
          *polya = POLY43A;
          *polyb = POLY43B;
          *polyc = POLY43C;
          *polyd = POLY43D;
          break;
        case 4:
          *polya = POLY44A;
          *polyb = POLY44B;
          *polyc = POLY44C;
          *polyd = POLY44D;
          break;
        case 5:
          *polya = POLY45A;
          *polyb = POLY45B;
          *polyc = POLY45C;
          *polyd = POLY45D;
          break;
        case 6:
          *polya = POLY46A;
          *polyb = POLY46B;
          *polyc = POLY46C;
          *polyd = POLY46D;
          break;
        case 7:
          *polya = POLY47A;
          *polyb = POLY47B;
          *polyc = POLY47C;
          *polyd = POLY47D;
          break;
        case 8:
          *polya = POLY48A;
          *polyb = POLY48B;
          *polyc = POLY48C;
          *polyd = POLY48D;
          break;
        case 9:
          *polya = POLY49A;
          *polyb = POLY49B;
          *polyc = POLY49C;
          *polyd = POLY49D;
          break;
        default: printf ("\n invalid value for K \n");  
      }
      break;
    default: printf ("\n invalid value for RATE \n");
      
  }
}
