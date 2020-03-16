/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: linked_list.h
 * Creation_Date: 29/03/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Thread-Safe Linked List implementation
 * 
 */
#ifndef __CSIRO_CORE_LINKED_LIST
#define __CSIRO_CORE_LINKED_LIST
/* Includes -------------------------------------------------*/

#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"

/* Module Defines -------------------------------------------*/

// clang-format off

#define LL_PROTECTED_OPERATION(op)	xSemaphoreTake(pxList->xAccess, portMAX_DELAY); op xSemaphoreGive(pxList->xAccess);

// clang-format on

/* Type Definitions -----------------------------------------*/

typedef struct xLinkedListItem_t
{
	struct xLinkedListItem_t *pxNext;
	struct xLinkedListItem_t *pxPrev;
} xLinkedListItem_t;

typedef struct xLinkedList_t
{
	SemaphoreHandle_t  xAccess;
	xLinkedListItem_t *pxHead;
	xLinkedListItem_t *pxTail;
	StaticSemaphore_t  xAccessStorage;
} xLinkedList_t;

/* Function Declarations ------------------------------------*/

/**
 * Initialises the Linked List
 * Can also be called to reset the list
 * 
 * \param pxList The Linked List
 */
void vLinkedListInit( xLinkedList_t *pxList );

/**
 * Checks if the Linked List is empty
 * 
 * \param pxList The Linked List
 * \return True if empty
 */
static inline bool bLinkedListEmpty( xLinkedList_t *pxList )
{
	LL_PROTECTED_OPERATION( bool bRet = ( pxList->pxHead == NULL ); )
	return bRet;
}

/**
 * Checks if the Linked List contains a single item
 * 
 * \param pxList The Linked List
 * \return True if list contains exactly one item
 */
static inline bool bLinkedListSingle( xLinkedList_t *pxList )
{
	LL_PROTECTED_OPERATION( bool bRet = ( pxList->pxHead != NULL ) && ( pxList->pxHead == pxList->pxTail ); )
	return bRet;
}

/**
 * Add a new item to the back of the linked list
 * 
 * \param pxList The Linked List
 * \param pxItem The item to add
 */
void vLinkedListAddToBack( xLinkedList_t *pxList, xLinkedListItem_t *pxItem );

/**
 * Add a new item to the front of the linked list
 * 
 * \param pxList The Linked List
 * \param pxItem The item to add
 */
void vLinkedListAddToFront( xLinkedList_t *pxList, xLinkedListItem_t *pxItem );

/**
 * Remove an item from the linked list
 * No attempt is made to validate that the item is a member of the list
 * 
 * \param pxList The Linked List
 * \param pxItem The item to remove
 */
void vLinkedListRemoveItem( xLinkedList_t *pxList, xLinkedListItem_t *pxItem );

/**
 * Get the current Linked List head
 * 
 * \param pxList The Linked List
 * \return pxItem The list head
 */
static inline xLinkedListItem_t *pxLinkedListHead( xLinkedList_t *pxList )
{
	LL_PROTECTED_OPERATION( xLinkedListItem_t *pxRet = pxList->pxHead; )
	return pxRet;
}

/**
 * Get the current Linked List tail
 * 
 * \param pxList The Linked List
 * \return pxItem The list tail
 */
static inline xLinkedListItem_t *pxLinkedListTail( xLinkedList_t *pxList )
{
	LL_PROTECTED_OPERATION( xLinkedListItem_t *pxRet = pxList->pxTail; )
	return pxRet;
}

/**
 * Check if the given item is the list head
 * 
 * \param pxList The Linked List
 * \param pxItem Item to check
 * \return  True when provided item is the head
 */
static inline bool bLinkedListIsHead( xLinkedList_t *pxList, xLinkedListItem_t *pxItem )
{
	LL_PROTECTED_OPERATION( bool bRet = pxList->pxHead == pxItem; )
	return bRet;
}

/**
 * Check if the given item is the list tail
 * 
 * \param pxList The Linked List
 * \param pxItem Item to check
 * \return  True when provided item is the tail
 */
static inline bool bLinkedListIsTail( xLinkedList_t *pxList, xLinkedListItem_t *pxItem )
{
	LL_PROTECTED_OPERATION( bool bRet = pxList->pxTail == pxItem; )
	return bRet;
}

/**
 * Get the next item in the Linked List
 * 
 * \param pxItem The current list item
 * \return pxItem The next list item
 */
static inline xLinkedListItem_t *pxLinkedListNextItem( xLinkedList_t *pxList, xLinkedListItem_t *pxItem )
{
	LL_PROTECTED_OPERATION( xLinkedListItem_t *pxRet = ( pxItem == NULL ) ? NULL : pxItem->pxNext; )
	return pxRet;
}

/**
 * Get the next item in the Linked List
 * 
 * \param pxItem The current list item
 * \return pxItem The next list item
 */
static inline xLinkedListItem_t *pxLinkedListPrevItem( xLinkedList_t *pxList, xLinkedListItem_t *pxItem )
{
	LL_PROTECTED_OPERATION( xLinkedListItem_t *pxRet = ( pxItem == NULL ) ? NULL : pxItem->pxPrev; )
	return pxRet;
}

#endif /* __CSIRO_CORE_LINKED_LIST */
