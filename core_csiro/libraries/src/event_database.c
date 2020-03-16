/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "event_database.h"

#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vEventDatabaseInit( xEventDatabase_t *pxDatabase )
{
	configASSERT( pxDatabase->ucNumEvents <= 24 );

	pxDatabase->xAccess		   = xSemaphoreCreateMutex();
	pxDatabase->xPendingEvents = xEventGroupCreate();
	pxDatabase->pucMemory	  = pvPortMalloc( pxDatabase->ucNumEvents * pxDatabase->ucDataSize );
}

/*-----------------------------------------------------------*/

void vEventDatabaseAdd( xEventDatabase_t *pxDatabase, uint8_t ucEventId, bool bOverwrite, const void *pvEventData )
{
	configASSERT( ucEventId < pxDatabase->ucNumEvents );
	configASSERT( pxDatabase->pucMemory );
	const uint32_t ulEventMask = 0b1 << ucEventId;

	/* Check if we're not overwriting, but an event already exists */
	if ( !bOverwrite && ( ulEventMask & xEventGroupGetBits( pxDatabase->xPendingEvents ) ) ) {
		return;
	}
	/* Protect our direct access and modification of RAM */
	xSemaphoreTake( pxDatabase->xAccess, portMAX_DELAY );
	/* Copy the event data into database */
	uint8_t *pucDatabaseEvent = pxDatabase->pucMemory + ( ucEventId * pxDatabase->ucDataSize );
	pvMemcpy( pucDatabaseEvent, pvEventData, pxDatabase->ucDataSize );
	/* Set the bits to notify that an event is present */
	xEventGroupSetBits( pxDatabase->xPendingEvents, ulEventMask );
	/* Release control of database */
	xSemaphoreGive( pxDatabase->xAccess );
}

/*-----------------------------------------------------------*/

uint8_t ucEventDatabaseWait( xEventDatabase_t *pxDatabase, uint8_t ucEventId, void *pvEventData, TickType_t xTimeout )
{
	configASSERT( ( ucEventId < pxDatabase->ucNumEvents ) || ( ucEventId == EVENT_ID_ANY ) );
	configASSERT( pxDatabase->pucMemory );
	const uint32_t ulEventMask = ( ucEventId == UINT8_MAX ) ? 0xFFFFFF : 0b1 << ucEventId;

	const EventBits_t xBits = xEventGroupWaitBits( pxDatabase->xPendingEvents, ulEventMask, pdFALSE, pdTRUE, xTimeout );
	if ( ulEventMask & xBits ) {
		/* Of the bits that were set, get the index of the first one */
		const uint8_t ulEventToHandle = FIND_FIRST_SET( ulEventMask & xBits ) - 1;
		/* Event appeared, protect our reading of RAM */
		xSemaphoreTake( pxDatabase->xAccess, portMAX_DELAY );

		/* Memory location of our event */
		uint8_t *pucDatabaseEvent = pxDatabase->pucMemory + ( ulEventToHandle * pxDatabase->ucDataSize );
		pvMemcpy( pvEventData, pucDatabaseEvent, pxDatabase->ucDataSize );
		/* Clear the event as we have now taken it */
		xEventGroupClearBits( pxDatabase->xPendingEvents, 0b1 << ulEventToHandle );
		/* Release control of database */
		xSemaphoreGive( pxDatabase->xAccess );
		return ulEventToHandle;
	}
	return EVENT_ID_NONE;
}

/*-----------------------------------------------------------*/
