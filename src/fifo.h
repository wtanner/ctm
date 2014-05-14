/*
*******************************************************************************
*
*      
*
*******************************************************************************
*
*      File             : fifo.h
*      Author           : Matthias Doerbecker
*      Tested Platforms : DEC Alpha 250 4/266 (OSF/1 3.2)
*      Description      : File I/O for float and double (32/64 bit signed) 
*                         values. 
*
*      Revision history
*
*      Rev  Date       Name            Description
*      -------------------------------------------------------------------
*      pA1  12-MAR-97  M.Doerbecker    initial version
*                                      (based on shortio.h pA2)
*
*******************************************************************************
*/
#ifndef fifo_h
#define fifo_h "$Id: $"

/*
*******************************************************************************
*                         INCLUDE FILES
*******************************************************************************
*/

#include "typedefs.h" /* basic data type aliases */
/*
*******************************************************************************
*                         DEFINITION OF CONSTANTS
*******************************************************************************
*/

/*
*******************************************************************************
*                         DEFINITION OF DATA TYPES
*******************************************************************************
*/


typedef enum {SHORTINT_FIFO, FLOAT_FIFO, CHAR_FIFO} fifo_type_t;


/* fifo_state_t is the state variable type that is used for both types */
/* of fifo structures: Shortint_fifo as well as Float_fifo.            */

typedef struct
{
  Shortint    *buffer_Shortint;  /* buffer for Shortint_fifo_xxx()           */
  Float       *buffer_Float;     /* buffer for Float_fifo_xxx()              */
  Char        *buffer_Char;      /* buffer for Char_fifo_xxx()               */
  Longint     length_buffer;     /* maximum length of the fifo buffer        */
  Longint     num_entries_actual;/* actual number of elements in fifo        */
  fifo_type_t fifo_type;         /* either SHORTINT_FIFO or FLOAT_FIFO       */
  Longint     magic_number;      /* for detecting wheter fifo is initialized */
}
fifo_state_t;


/*
*******************************************************************************
*                         DECLARATION OF PROTOTYPES
*******************************************************************************
*/


/*
*******************************************************************************
*
*     Function        : Shortint_fifo_init
*     In              : length_fifo   determines maximukm length of the buffer
*     Out             : fifo_state    initialized state variable
*     Calls           : calloc, fprintf, exit
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : initialization of a fifo structure for buffering 
*                       Shortint data
*
*******************************************************************************
*/

int Shortint_fifo_init(fifo_state_t *fifo_state,
                       Longint      length_fifo);



/*
*******************************************************************************
*
*     Function        : Shortint_fifo_reset
*     In              : 
*     In/Out          : fifo_state    initialized state variable
*     Calls           : <none>
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : reset of the fifo structure, i.e. all buffered 
*                       elements are removed
*
*******************************************************************************
*/

int Shortint_fifo_reset(fifo_state_t *fifo_state);



/*
*******************************************************************************
*
*     Function        : Shortint_fifo_exit
*     In              : length_fifo   determines maximum length of the buffer
*     In/Out          : fifo_state    initialized state variable
*     Calls           : free, fprintf, exit
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : shuts down fifo structure, frees allocated memory
*
*******************************************************************************
*/

int Shortint_fifo_exit(fifo_state_t *fifo_state);



/*
*******************************************************************************
*
*     Function        : Shortint_fifo_push
*     In              : elements_to_push        vector containing the elements
*     In                num_elements_to_push    number of the elements
*     In/Out          : fifo_state              state variable
*     Calls           : fprintf, exit
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : pushes elements into the fifo
*
*******************************************************************************
*/

int Shortint_fifo_push(fifo_state_t *fifo_state, 
                       Shortint     *elements_to_push,
                       Longint      num_elements_to_push);


/*
*******************************************************************************
*
*     Function        : Shortint_fifo_pop
*     In              : num_elements_to_pop     number of the elements
*     Out             : popped_elements         vector containing the elements
*     In/Out          : fifo_state              state variable
*     Calls           : fprintf, exit
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : pops elements from the fifo
*
*******************************************************************************
*/

int Shortint_fifo_pop(fifo_state_t *fifo_state, 
                      Shortint     *popped_elements,
                      Longint      num_elements_to_pop);


/*
*******************************************************************************
*
*     Function        : Shortint_fifo_peek
*     In              : num_elements_to_peek    number of the elements
*     Out             : peeked_elements         vector containing the elements
*     In/Out          : fifo_state              state variable
*     Calls           : fprintf, exit
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : similar to pop, but elements are remaining in buffer
*
*******************************************************************************
*/

int Shortint_fifo_peek(fifo_state_t *fifo_state, 
                       Shortint     *peeked_elements,
                       Longint      num_elements_to_peek);


/*
*******************************************************************************
*
*     Function        : Shortint_fifo_check
*     In/Out          : fifo_state              state variable
*     Calls           : <none>
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : number of elements in fifo
*     Information     : determines the number of valid elements in fifo
*
*******************************************************************************
*/

Longint Shortint_fifo_check(fifo_state_t *fifo_state);






/*
*******************************************************************************
*
*     Function        : Float_fifo_init
*     In              : length_fifo   determines maximukm length of the buffer
*     Out             : fifo_state    initialized state variable
*     Calls           : calloc, fprintf, exit
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : initialization of a fifo structure for buffering 
*                       Float data
*
*******************************************************************************
*/

int Float_fifo_init(fifo_state_t *fifo_state,
                    Longint      length_fifo);



/*
*******************************************************************************
*
*     Function        : Float_fifo_reset
*     In              : 
*     In/Out          : fifo_state    initialized state variable
*     Calls           : <none>
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : reset of the fifo structure, i.e. all buffered 
*                       elements are removed
*
*******************************************************************************
*/

int Float_fifo_reset(fifo_state_t *fifo_state);



/*
*******************************************************************************
*
*     Function        : Float_fifo_exit
*     In              : length_fifo   determines maximum length of the buffer
*     In/Out          : fifo_state    initialized state variable
*     Calls           : free, fprintf, exit
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : shuts down fifo structure, frees allocated memory
*
*******************************************************************************
*/

int Float_fifo_exit(fifo_state_t *fifo_state);



/*
*******************************************************************************
*
*     Function        : Float_fifo_push
*     In              : elements_to_push        vector containing the elements
*     In                num_elements_to_push    number of the elements
*     In/Out          : fifo_state              state variable
*     Calls           : fprintf, exit
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : pushes elements into the fifo
*
*******************************************************************************
*/

int Float_fifo_push(fifo_state_t *fifo_state, 
                    Float     *elements_to_push,
                    Longint      num_elements_to_push);


/*
*******************************************************************************
*
*     Function        : Float_fifo_pop
*     In              : num_elements_to_pop     number of the elements
*     Out             : popped_elements         vector containing the elements
*     In/Out          : fifo_state              state variable
*     Calls           : fprintf, exit
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : 0 on success, 1 in case of an error
*     Information     : pops elements from the fifo
*
*******************************************************************************
*/

int Float_fifo_pop(fifo_state_t *fifo_state, 
                   Float     *popped_elements,
                   Longint      num_elements_to_pop);


/*
*******************************************************************************
*
*     Function        : Float_fifo_check
*     In/Out          : fifo_state              state variable
*     Calls           : <none>
*     Tables          : <none>
*     Compile Defines : <none>
*     Return          : number of elements in fifo
*     Information     : determines the number of valid elements in fifo
*
*******************************************************************************
*/

Longint Float_fifo_check(fifo_state_t *fifo_state);



/***********************************************************************/
/* Now the same functions for type Char                                */
/***********************************************************************/

int Char_fifo_init(fifo_state_t *fifo_state,
                   Longint      length_fifo);

int Char_fifo_reset(fifo_state_t *fifo_state);

int Char_fifo_exit(fifo_state_t *fifo_state);

int Char_fifo_push(fifo_state_t *fifo_state, 
                   Char         *elements_to_push,
                   Longint       num_elements_to_push);

int Char_fifo_pop(fifo_state_t *fifo_state, 
                  Char         *popped_elements,
                  Longint       num_elements_to_pop);

Longint Char_fifo_check(fifo_state_t *fifo_state);

#endif
