/*
*******************************************************************************
*
*      *******************************************************************************
*
*      File             : baudot_functions.h
*      Author           : EEDN/RV Matthias Doerbecker
*      Tested Platforms : Windows NT 4.0
*      Description      : header file for ucs_functions.c
*
*      Revision history
*
*      $Log: $
*
*******************************************************************************
*/
#ifndef ucs_functions_h
#define ucs_functions_h "$Id: $"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include "fifo.h"
#include <typedefs.h>

/*
*******************************************************************************
*                         DEFINITIONS
*******************************************************************************
*/

/****************************************************************************/
/* convertChar2UCScode()                                                    */
/* *********************                                                    */
/* Conversion from character into UCS code                                  */
/* (Universal Multiple-Octet Coded Character Set, Row 00                    */
/* of the Multilingual plane according to ISO/IEC 10646-1).                 */
/* This routine only handles characters in the range 0...255 since that is  */
/* all that is required for the demonstration of Baudot support.            */
/*                                                                          */
/* input variables:                                                         */
/* - inChar       charcater that shall be converted                         */
/*                                                                          */
/* return value:  UCS code of the input character                           */
/*                or 0xFFFF in case that inChar is not valid                */
/*                (e.g. inChar=='\0')                                       */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/09/18 */
/****************************************************************************/

UShortint convertChar2UCScode(char inChar);


/****************************************************************************/
/* convertUCScode2char()                                                    */
/* *********************                                                    */
/* Conversion from UCS code into character                                  */
/* This routine only handles characters in the range 0...255 since that is  */
/* all that is required for the demonstration of Baudot support.            */
/*                                                                          */
/* input variables:                                                         */
/* - ucsCode      UCS code index,                                           */
/*                must be within the range 0...255 if BITS_PER_SYMB==8,     */
/*                or in the range 0...63 if BITS_PER_SYMB==6,               */
/*                                                                          */
/* return value:  character (or '\0' if ucsCode is not valid)               */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/09/18 */
/****************************************************************************/

char convertUCScode2char(UShortint ucsCode);


/****************************************************************************/
/* transformUCS2UTF()                                                       */
/* ******************                                                       */
/* Transformation from UCS code into UTF-8. UTF-8 is a sequence consisting  */
/* of 1, 2, 3, or 5 octets (bytes). See ISO/IEC 10646-1 Annex G.            */
/*                                                                          */
/* This routine only handles UCS codes in the range 0...0xFF since that is  */
/* all that is required for the demonstration of Baudot support.            */
/*                                                                          */
/* input variables:                                                         */
/* - ucsCode               UCS code index,                                  */
/*                                                                          */
/* output variables:                                                        */
/* - ptr_octet_fifo_state  pointer to the output fifo state buffer for      */
/*                         the UTF-8 octets.                                */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/06/29 */
/****************************************************************************/

void transformUCS2UTF(UShortint      ucsCode,
                      fifo_state_t*  ptr_octet_fifo_state);


/****************************************************************************/
/* transformUTF2UCS()                                                       */
/* ******************                                                       */
/* Transformation from UTF-8 into UCS code.                                 */
/*                                                                          */
/* This routine only handles UTF-8 sequences consisting of one or two       */
/* octets (corresponding to UCS codes in the range 0...0xFF) since that is  */
/* all that is required for the demonstration of Baudot support.            */
/*                                                                          */
/* input/output variables:                                                  */
/* - ptr_octet_fifo_state  pointer to the input fifo state buffer for       */
/*                         the UTF-8 octets.                                */
/*                                                                          */
/* output variables:                                                        */
/* - *ptr_ucsCode          UCS code index                                   */
/*                                                                          */
/* return value:                                                            */
/* true,  if conversion was successful                                      */
/* false, if the input fifo buffer didn't contain enough octets for a       */
/*        conversion into UCS code. The output variable *ptr_ucsCode        */
/*        doesn't contain a valid value in this case                        */
/*                                                                          */
/* Matthias Doerbecker, Ericsson Eurolab Deutschland (EED/N/RV), 2000/06/29 */
/****************************************************************************/

Bool transformUTF2UCS(UShortint     *ptr_ucsCode,
                      fifo_state_t  *ptr_octet_fifo_state);


#endif
