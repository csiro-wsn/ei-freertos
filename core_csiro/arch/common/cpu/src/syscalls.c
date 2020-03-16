/**************************************************************************/ /**
 * @brief Exit the program.
 * @param[in] status The value to return to the parent process as the
 *            exit status (not used).
 *****************************************************************************/

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "compiler_intrinsics.h"

/* Function Declarations ------------------------------------*/

/* Variables  -----------------------------------------------*/

/* Functions ------------------------------------------------*/

void _exit( int status )
{
	(void) status;
	while ( 1 ) continue;
}

/*-----------------------------------------------------------*/

/* Why this is called by the bluetooth stack is not clear */
char *_sbrk( int incr )
{
	return pvPortMalloc( incr );
}

/*-----------------------------------------------------------*/
