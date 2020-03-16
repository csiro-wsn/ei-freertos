/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "linked_list.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vLinkedListInit( xLinkedList_t *pxList )
{
	pxList->xAccess = xSemaphoreCreateBinaryStatic( &pxList->xAccessStorage );
	xSemaphoreGive( pxList->xAccess );
	pxList->pxHead = NULL;
	pxList->pxTail = NULL;
}

/*-----------------------------------------------------------*/

void vLinkedListAddToBack( xLinkedList_t *pxList, xLinkedListItem_t *pxItem )
{
	xSemaphoreTake( pxList->xAccess, portMAX_DELAY );
	/* The previous item is the old tail */
	pxItem->pxPrev = pxList->pxTail;
	/* Last item in the list */
	pxItem->pxNext = NULL;
	/* Update the old tails next */
	if ( pxList->pxTail != NULL ) {
		pxList->pxTail->pxNext = pxItem;
	}
	/* The tail of the list is now this item */
	pxList->pxTail = pxItem;
	/* If the list is empty, this item is also the head */
	if ( pxList->pxHead == NULL ) {
		pxList->pxHead = pxItem;
	}
	xSemaphoreGive( pxList->xAccess );
}

/*-----------------------------------------------------------*/

void vLinkedListAddToFront( xLinkedList_t *pxList, xLinkedListItem_t *pxItem )
{
	xSemaphoreTake( pxList->xAccess, portMAX_DELAY );
	/* First item in the list */
	pxItem->pxPrev = NULL;
	/* Previous item is new tail */
	pxItem->pxNext = pxList->pxHead;
	/* Update the old heads prev */
	if ( pxList->pxHead != NULL ) {
		pxList->pxHead->pxPrev = pxItem;
	}
	/* The head of the list is now this item */
	pxList->pxHead = pxItem;
	/* If the list is empty, this item is also the tail */
	if ( pxList->pxTail == NULL ) {
		pxList->pxTail = pxItem;
	}
	xSemaphoreGive( pxList->xAccess );
}

/*-----------------------------------------------------------*/

void vLinkedListRemoveItem( xLinkedList_t *pxList, xLinkedListItem_t *pxItem )
{
	xSemaphoreTake( pxList->xAccess, portMAX_DELAY );
	if ( pxList->pxHead == pxItem ) {
		pxList->pxHead = pxItem->pxNext;
	}
	if ( pxList->pxTail == pxItem ) {
		pxList->pxTail = pxItem->pxPrev;
	}
	if ( pxItem->pxPrev != NULL ) {
		pxItem->pxPrev->pxNext = pxItem->pxNext;
	}
	if ( pxItem->pxNext != NULL ) {
		pxItem->pxNext->pxPrev = pxItem->pxPrev;
	}
	xSemaphoreGive( pxList->xAccess );
}

/*-----------------------------------------------------------*/
