/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : fifo.c
*      Author           : Matthias Doerbecker
*      Tested Platforms : Sun Solaris
*      Description      : Fifo structures for Shortint and Float
*
*      Revision history
*
*      Rev  Date       Name            Description
*      -------------------------------------------------------------------
*      pA1  14-Dec-99  M.Doerbecker    initial version
*
*******************************************************************************
*/

/*
*******************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*******************************************************************************
*/
const char fifo_id[] = "@(#)$Id: $";

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/
#include <stdlib.h>    /* malloc, free */
#include <stdio.h>
#include "fifo.h"

#include <typedefs.h>  /* basic data type aliases */


/*
*******************************************************************************
*                         LOCAL VARIABLES AND TABLES
*******************************************************************************
*/


/*
*******************************************************************************
*                         LOCAL PROGRAM CODE
*******************************************************************************
*/


int Shortint_fifo_init(fifo_state_t *fifo_state,
                       Longint length_fifo)
{
  if (length_fifo<1)
    {
      fprintf(stderr, "Error using Shortint_fifo_init; length_fifo ");
      fprintf(stderr, "must be greater than 0!\n\n");
      exit(1);
    }
  
  fifo_state->buffer_Shortint 
    = (Shortint*)calloc(length_fifo, sizeof(Shortint));
  
  if (fifo_state->buffer_Shortint == (Shortint*)NULL)
    {
      fprintf(stderr, 
              "Error while allocating memory for Shortint_fifo_init!\n\n");
      exit(1);
    }
  
  fifo_state->buffer_Float       = (Float*)NULL;
  fifo_state->buffer_Char        = (Char*)NULL;
  fifo_state->length_buffer      = length_fifo;
  fifo_state->num_entries_actual = 0;
  fifo_state->fifo_type          = SHORTINT_FIFO;
  fifo_state->magic_number       = 12345;
  return 0;
}

/************************************************************************/

int Shortint_fifo_reset(fifo_state_t *fifo_state)
{
  fifo_state->num_entries_actual = 0;
  return 0;
}

/************************************************************************/

int Shortint_fifo_exit(fifo_state_t *fifo_state)
{
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Shortint_fifo_exit(): \n");
      fprintf(stderr, "Fifo must be initialized before exit!\n\n");
      exit(1);
    }
  
  free(fifo_state->buffer_Shortint);
  free(fifo_state->buffer_Float);
  fifo_state->buffer_Shortint    = (Shortint*)NULL;
  fifo_state->buffer_Float       = (Float*)NULL;
  fifo_state->num_entries_actual = 0;
  return 0;
}
  
/************************************************************************/

int Shortint_fifo_push(fifo_state_t *fifo_state, 
                       Shortint     *elements_to_push,
                       Longint      num_elements_to_push)
{
  Longint cnt;
  
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Shortint_fifo_push(): \n");
      fprintf(stderr, "Fifo must be initialized before use!\n\n");
      exit(1);
    }
  
  if (fifo_state->fifo_type != SHORTINT_FIFO) 
    {
      fprintf(stderr, "Function Shortint_fifo_push(): \n");
      fprintf(stderr, "Initialization was not of type SHORTINT_FIFO!\n\n");
      exit(1);
    }

  if (num_elements_to_push+fifo_state->num_entries_actual > 
      fifo_state->length_buffer)
    {
      fprintf(stderr, "Function Shortint_fifo_push(): \n");
      fprintf(stderr, "Overflow while pushing %d ", num_elements_to_push);
      fprintf(stderr, "elements into buffer (buffer size=%d)\n\n", 
              fifo_state->length_buffer);
      exit(1);
    }
  
  /* append new elements at the end of the buffer */
  for (cnt=0; cnt<num_elements_to_push; cnt++)
    fifo_state->buffer_Shortint[fifo_state->num_entries_actual+cnt] 
      = elements_to_push[cnt];
  fifo_state->num_entries_actual += num_elements_to_push;
  return 0;
}

/************************************************************************/

int Shortint_fifo_pop(fifo_state_t *fifo_state, 
                      Shortint     *popped_elements,
                      Longint      num_elements_to_pop)
{
  Longint cnt;
  
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Shortint_fifo_pop(): \n");
      fprintf(stderr, "Fifo must be initialized before exit!\n\n");
      exit(1);
    }
  
  if (fifo_state->fifo_type != SHORTINT_FIFO) 
    {
      fprintf(stderr, "Function Shortint_fifo_pop(): \n");
      fprintf(stderr, "Initialization was not of type SHORTINT_FIFO!\n\n");
      exit(1);
    }

  if (num_elements_to_pop > fifo_state->num_entries_actual)
    {
      fprintf(stderr, "Function Shortint_fifo_pop(): \nBuffer underrun ");
      fprintf(stderr, "while popping %d ", num_elements_to_pop);
      fprintf(stderr, "elements; only ");
      fprintf(stderr, "%d elements in buffer!\n\n", 
              fifo_state->num_entries_actual);
      exit(1);
    }
  
  /* read out first (oldest) element from the buffer */
  for (cnt=0; cnt<num_elements_to_pop; cnt++)
    popped_elements[cnt] = fifo_state->buffer_Shortint[cnt];
  
  fifo_state->num_entries_actual -= num_elements_to_pop;

  /* shift whole buffer to the left */
  for (cnt=0; cnt<fifo_state->num_entries_actual; cnt++)
    fifo_state->buffer_Shortint[cnt] = 
      fifo_state->buffer_Shortint[cnt+num_elements_to_pop];
  
  return(0);
}

/************************************************************************/

int Shortint_fifo_peek(fifo_state_t *fifo_state, 
                       Shortint     *peeked_elements,
                       Longint      num_elements_to_peek)
{
  Longint cnt;
  
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Shortint_fifo_peek(): \n");
      fprintf(stderr, "Fifo must be initialized before exit!\n\n");
      exit(1);
    }
  
  if (fifo_state->fifo_type != SHORTINT_FIFO) 
    {
      fprintf(stderr, "Function Shortint_fifo_peek(): \n");
      fprintf(stderr, "Initialization was not of type SHORTINT_FIFO!\n\n");
      exit(1);
    }

  if (num_elements_to_peek > fifo_state->num_entries_actual)
    {
      fprintf(stderr, "Function Shortint_fifo_peek(): \n");
      fprintf(stderr, "Trying to peek %d ", num_elements_to_peek);
      fprintf(stderr, "elements; only ");
      fprintf(stderr, "%d elements in buffer!\n\n", 
              fifo_state->num_entries_actual);
      exit(1);
    }
  
  /* read out first (oldest) element from the buffer */
  for (cnt=0; cnt<num_elements_to_peek; cnt++)
    peeked_elements[cnt] = fifo_state->buffer_Shortint[cnt];
  
  return(0);
}

/************************************************************************/
  
Longint Shortint_fifo_check(fifo_state_t *fifo_state)
{
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Shortint_fifo_check(): \n");
      fprintf(stderr, "Fifo must be initialized before use!\n\n");
      exit(1);
    }
  
  return(fifo_state->num_entries_actual);
}

/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/

int Float_fifo_init(fifo_state_t *fifo_state,
                    Longint length_fifo)
{
  if (length_fifo<1)
    {
      fprintf(stderr, "Error using Float_fifo_init; length_fifo ");
      fprintf(stderr, "must be greater than 0!\n\n");
      exit(1);
    }
  
  fifo_state->buffer_Float 
    = (Float*)calloc(length_fifo, sizeof(Float));
  
  if (fifo_state->buffer_Float == (Float*)NULL)
    {
      fprintf(stderr, 
              "Error while allocating memory for Float_fifo_init!\n\n");
      exit(1);
    }
  
  fifo_state->buffer_Shortint    = (Shortint*)NULL;
  fifo_state->buffer_Char        = (Char*)NULL;
  fifo_state->length_buffer      = length_fifo;
  fifo_state->num_entries_actual = 0;
  fifo_state->fifo_type          = FLOAT_FIFO;
  fifo_state->magic_number       = 12345;
  return 0;
}

/************************************************************************/

int Float_fifo_reset(fifo_state_t *fifo_state)
{
  fifo_state->num_entries_actual = 0;
  return 0;
}

/************************************************************************/

int Float_fifo_exit(fifo_state_t *fifo_state)
{
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Float_fifo_exit(): \n");
      fprintf(stderr, "Fifo must be initialized before exit!\n\n");
      exit(1);
    }
  
  free(fifo_state->buffer_Shortint);
  free(fifo_state->buffer_Float);
  fifo_state->buffer_Shortint    = (Shortint*)NULL;
  fifo_state->buffer_Float       = (Float*)NULL;
  fifo_state->num_entries_actual = 0;
  return 0;
}
  
/************************************************************************/

int Float_fifo_push(fifo_state_t *fifo_state, 
                    Float     *elements_to_push,
                    Longint      num_elements_to_push)
{
  Longint cnt;
  
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Float_fifo_push(): \n");
      fprintf(stderr, "Fifo must be initialized before use!\n\n");
      exit(1);
    }
  
  if (fifo_state->fifo_type != FLOAT_FIFO) 
    {
      fprintf(stderr, "Function Float_fifo_push(): \n");
      fprintf(stderr, "Initialization was not of type FLOAT_FIFO!\n\n");
      exit(1);
    }

  if (num_elements_to_push+fifo_state->num_entries_actual > 
      fifo_state->length_buffer)
    {
      fprintf(stderr, "Function Float_fifo_push(): \n");
      fprintf(stderr, "Overflow while pushing %d ", num_elements_to_push);
      fprintf(stderr, "elements into buffer (buffer size=%d)\n\n", 
              fifo_state->length_buffer);
      exit(1);
    }
  
  /* append new elements at the end of the buffer */
  for (cnt=0; cnt<num_elements_to_push; cnt++)
    fifo_state->buffer_Float[fifo_state->num_entries_actual+cnt] 
      = elements_to_push[cnt];
  fifo_state->num_entries_actual += num_elements_to_push;
  return 0;
}

/************************************************************************/

int Float_fifo_pop(fifo_state_t *fifo_state, 
                   Float     *popped_elements,
                   Longint      num_elements_to_pop)
{
  Longint cnt;
  
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Float_fifo_pop(): \n");
      fprintf(stderr, "Fifo must be initialized before exit!\n\n");
      exit(1);
    }
  
  if (fifo_state->fifo_type != FLOAT_FIFO) 
    {
      fprintf(stderr, "Function Float_fifo_pop(): \n");
      fprintf(stderr, "Initialization was not of type FLOAT_FIFO!\n\n");
      exit(1);
    }

  if (num_elements_to_pop > fifo_state->num_entries_actual)
    {
      fprintf(stderr, "Function Float_fifo_pop(): \nBuffer underrun ");
      fprintf(stderr, "while popping %d ", num_elements_to_pop);
      fprintf(stderr, "elements; only ");
      fprintf(stderr, "%d elements in buffer!\n\n", 
              fifo_state->num_entries_actual);
      exit(1);
    }
  
  /* read out first (oldest) element from the buffer */
  for (cnt=0; cnt<num_elements_to_pop; cnt++)
    popped_elements[cnt] = fifo_state->buffer_Float[cnt];
  
  fifo_state->num_entries_actual -= num_elements_to_pop;

  /* shift whole buffer to the left */
  for (cnt=0; cnt<fifo_state->num_entries_actual; cnt++)
    fifo_state->buffer_Float[cnt] = 
      fifo_state->buffer_Float[cnt+num_elements_to_pop];
  
  return(0);
}

/************************************************************************/
  
Longint Float_fifo_check(fifo_state_t *fifo_state)
{
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Float_fifo_check(): \n");
      fprintf(stderr, "Fifo must be initialized before use!\n\n");
      exit(1);
    }
  
  return(fifo_state->num_entries_actual);
}

/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/

int Char_fifo_init(fifo_state_t *fifo_state,
                   Longint length_fifo)
{
  if (length_fifo<1)
    {
      fprintf(stderr, "Error using Char_fifo_init; length_fifo ");
      fprintf(stderr, "must be greater than 0!\n\n");
      exit(1);
    }
  
  fifo_state->buffer_Char 
    = (Char*)calloc(length_fifo, sizeof(Char));
  
  if (fifo_state->buffer_Char == (Char*)NULL)
    {
      fprintf(stderr, 
              "Error while allocating memory for Char_fifo_init!\n\n");
      exit(1);
    }
  
  fifo_state->buffer_Shortint    = (Shortint*)NULL;
  fifo_state->buffer_Float       = (Float*)NULL;
  fifo_state->length_buffer      = length_fifo;
  fifo_state->num_entries_actual = 0;
  fifo_state->fifo_type          = CHAR_FIFO;
  fifo_state->magic_number       = 12345;
  return 0;
}

/************************************************************************/

int Char_fifo_reset(fifo_state_t *fifo_state)
{
  fifo_state->num_entries_actual = 0;
  return 0;
}

/************************************************************************/

int Char_fifo_exit(fifo_state_t *fifo_state)
{
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Char_fifo_exit(): \n");
      fprintf(stderr, "Fifo must be initialized before exit!\n\n");
      exit(1);
    }
  
  free(fifo_state->buffer_Shortint);
  free(fifo_state->buffer_Float);
  free(fifo_state->buffer_Char);
  fifo_state->buffer_Shortint    = (Shortint*)NULL;
  fifo_state->buffer_Float       = (Float*)NULL;
  fifo_state->buffer_Char        = (Char*)NULL;
  fifo_state->num_entries_actual = 0;
  return 0;
}

/************************************************************************/

int Char_fifo_push(fifo_state_t *fifo_state, 
                       Char     *elements_to_push,
                       Longint      num_elements_to_push)
{
  Longint cnt;
  
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Char_fifo_push(): \n");
      fprintf(stderr, "Fifo must be initialized before use!\n\n");
      exit(1);
    }
  
  if (fifo_state->fifo_type != CHAR_FIFO) 
    {
      fprintf(stderr, "Function Char_fifo_push(): \n");
      fprintf(stderr, "Initialization was not of type CHAR_FIFO!\n\n");
      exit(1);
    }

  if (num_elements_to_push+fifo_state->num_entries_actual > 
      fifo_state->length_buffer)
    {
      fprintf(stderr, "Function Char_fifo_push(): \n");
      fprintf(stderr, "Overflow while pushing %d ", num_elements_to_push);
      fprintf(stderr, "elements into buffer (buffer size=%d)\n\n", 
              fifo_state->length_buffer);
      exit(1);
    }
  
  /* append new elements at the end of the buffer */
  for (cnt=0; cnt<num_elements_to_push; cnt++)
    fifo_state->buffer_Char[fifo_state->num_entries_actual+cnt] 
      = elements_to_push[cnt];
  fifo_state->num_entries_actual += num_elements_to_push;
  return 0;
}

/************************************************************************/

int Char_fifo_pop(fifo_state_t *fifo_state, 
                      Char     *popped_elements,
                      Longint      num_elements_to_pop)
{
  Longint cnt;
  
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Char_fifo_pop(): \n");
      fprintf(stderr, "Fifo must be initialized before exit!\n\n");
      exit(1);
    }
  
  if (fifo_state->fifo_type != CHAR_FIFO) 
    {
      fprintf(stderr, "Function Char_fifo_push(): \n");
      fprintf(stderr, "Initialization was not of type CHAR_FIFO!\n\n");
      exit(1);
    }

  if (num_elements_to_pop > fifo_state->num_entries_actual)
    {
      fprintf(stderr, "Function Char_fifo_pop(): \nBuffer underrun ");
      fprintf(stderr, "while popping %d ", num_elements_to_pop);
      fprintf(stderr, "elements; only ");
      fprintf(stderr, "%d elements in buffer!\n\n", 
              fifo_state->num_entries_actual);
      exit(1);
    }
  
  /* read out first (oldest) element from the buffer */
  for (cnt=0; cnt<num_elements_to_pop; cnt++)
    popped_elements[cnt] = fifo_state->buffer_Char[cnt];
  
  fifo_state->num_entries_actual -= num_elements_to_pop;

  /* shift whole buffer to the left */
  for (cnt=0; cnt<fifo_state->num_entries_actual; cnt++)
    fifo_state->buffer_Char[cnt] = 
      fifo_state->buffer_Char[cnt+num_elements_to_pop];
  
  return(0);
}

/************************************************************************/
  
Longint Char_fifo_check(fifo_state_t *fifo_state)
{
  if (fifo_state->magic_number != 12345) 
    {
      fprintf(stderr, "Function Char_fifo_check(): \n");
      fprintf(stderr, "Fifo must be initialized before use!\n\n");
      exit(1);
    }
  
  return(fifo_state->num_entries_actual);
}

