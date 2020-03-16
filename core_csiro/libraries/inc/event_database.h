/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: event_database.h
 * Creation_Date: 01/10/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Thread safe mini database for storing distinct events in RAM
 */
#ifndef __CSIRO_CORE_EVENT_DATABASE
#define __CSIRO_CORE_EVENT_DATABASE
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define EVENT_ID_ANY    UINT8_MAX
#define EVENT_ID_NONE   (UINT8_MAX - 1)

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef struct xEventDatabase_t
{
	SemaphoreHandle_t  xAccess;		   /**< Access protection semaphore */
	EventGroupHandle_t xPendingEvents; /**< Events currently pending in the database */
	uint8_t			   ucNumEvents;	/**< Limited to a maximum of 24 */
	uint8_t			   ucDataSize;	 /**< Size of each event in bytes */
	uint8_t *		   pucMemory;	  /**< Database memory */
} xEventDatabase_t;

/* Function Declarations ------------------------------------*/

/**@brief Initialise event database
 *
 * @param[in] pxDatabase		    Database instance
 */
void vEventDatabaseInit( xEventDatabase_t *pxDatabase );

/**@brief Add an event to the database
 *
 * @param[in] pxDatabase		    Database instance
 * @param[in] ucEventId		        ID to store event against
 * @param[in] bOverwrite		    Overwrite existing events
 * @param[in] pvEventData		    Data associated with the event
 */
void vEventDatabaseAdd( xEventDatabase_t *pxDatabase, uint8_t ucEventId, bool bOverwrite, const void *pvEventData );

/**@brief Wait for an event to be added to the database
 *
 * @param[in] pxDatabase		    Database instance
 * @param[in] ucEventId		        Event ID to wait for
 *                                  EVENT_ID_ANY is the special "ANY EVENT" ID
 * @param[in] xTimeout		        Duration to wait for the event
 * 
 * @retval	EVENT_ID_NONE           Wait timed out
 * @retval	other                   Event ID that was written
 */
uint8_t ucEventDatabaseWait( xEventDatabase_t *pxDatabase, uint8_t ucEventId, void *pvEventData, TickType_t xTimeout );

#endif /* __CSIRO_CORE_EVENT_DATABASE */
