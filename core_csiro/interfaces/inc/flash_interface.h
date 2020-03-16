/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: flash_interface.h
 * Creation_Date: 06/08/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Common interface to Flash Memory peripherals
 * 
 */
#ifndef __CSIRO_CORE_FLASH_INTERFACE
#define __CSIRO_CORE_FLASH_INTERFACE
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "core_types.h"

/* Module Defines -------------------------------------------*/
// clang-format off

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef struct xFlashSettings_t		  xFlashSettings_t;
typedef struct xFlashImplementation_t xFlashImplementation_t;

/**@brief Generic parameterisation of a flash device */
struct xFlashSettings_t
{
	uint32_t ulNumPages;	   /**< Number of pages present on the device */
	uint16_t usPageSize;	   /**< Number of bytes in a page */
	uint16_t usErasePages;	 /**< Number of pages erased by the smallest erase operation */
	uint8_t  ucEraseByte;	  /**< The byte present after erasing (0x00 or 0xFF) */
	uint8_t  ucPageSizePower;  /**< 2 ^ (ucPageSizePower) == usPageSize */
	uint16_t usPageOffsetMask; /**< ( usPageSize - 1 ) == usPageOffsetMask */
	uint8_t *pucPage;		   /**< Memory region equal in size to a single flash page */
};

/**@brief Parent type of all flash hardware instances */
typedef struct xFlashDefaultHardware_t
{
	void *pvInterface; /**< Pointer to the communication interface, typically SPI */
} xFlashDefaultHardware_t;

/**@brief Flash Device Handle */
typedef struct xFlashDevice_t
{
	xFlashSettings_t		 xSettings;		   /**< Device Parameterisation */
	xFlashImplementation_t * pxImplementation; /**< Base functionality implementations*/
	QueueHandle_t			 xCommandQueue;	/**< Queue to push commands at */
	const char *			 pcName;		   /**< Unique device name*/
	xFlashDefaultHardware_t *pxHardware;	   /**< Device Specific Hardware Mappings */
} xFlashDevice_t;

/**@brief Initialise Flash peripheral and query parameters
 * 
 * @note 	This function must completely initialise pxDevice->xSettings
 * @note	pxDevice->pxHardware must be completely initialised between this function and board files
 * 
 * @param[in] pxDevice	    	Flash device to initialise
 * 
 * @retval::ERROR_NONE			Initialisation and querying successful
 */
typedef eModuleError_t ( *fnFlashInit_t )( xFlashDevice_t *pxDevice );

/**@brief Put the flash device into its lowest power mode
 * 
 * @param[in] pxDevice	    	Flash device to put to sleep
 * 
 * @retval::ERROR_NONE			Device put into lowest power mode
 */
typedef eModuleError_t ( *fnFlashSleep_t )( xFlashDevice_t *pxDevice );

/**@brief Bring the device out of its lowest power mode
 * 
 * The device may have been completely depowered after entering sleep mode.
 * Any initial setup must be performed again if bWasDepowered is true.
 * 
 * @param[in] pxDevice	    	Flash device to wake from sleep mode
 * @param[in] bWasDepowered	    True when power was removed from the device in sleep mode
 * 
 * @retval::ERROR_NONE			Device put into lowest power mode
 */
typedef eModuleError_t ( *fnFlashWake_t )( xFlashDevice_t *pxDevice, bool bWasDepowered );

/**@brief Write a single page to the device
 * 
 * @param[in] pxDevice	    	Flash device
 * @param[in] ulPage	        Page to read/write
 * @param[in] usPageOffset	    Offset into the page to read/write
 * @param[in] pucData	        Data to read/write from/to the page
 * @param[in] usDataLen	        Number of bytes to read/write
 * 
 * @retval::ERROR_NONE			Read/Write succeeded
 */
typedef eModuleError_t ( *fnFlashReadWriteSubpage_t )( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset, uint8_t *pucData, uint16_t usDataLen );

/**@brief Start a read at the provided address
 * 
 * Put the SPI bus in a state where reading bytes will read a continuous stream of bytes starting
 * at the provided memory location
 * 
 * @note 	SPI Bus is not released at the end of this function
 * @note	No data is actually read in this function
 * 
 * @param[in] pxDevice	    	Flash device
 * @param[in] ulPage	    	Page to begin read
 * @param[in] usPageOffset	    Offset to begin read
 * 
 * @retval::ERROR_NONE			Device erased
 */
typedef eModuleError_t ( *fnFlashReadStart_t )( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset );

/**@brief Erase pages from the device
 * 
 * @note ulStartPage and ulNumPages are gauranteed to be aligned to the minimum erase size of the device
 * 
 * @param[in] pxDevice	    	Flash device
 * @param[in] ulStartPage	    First page to erase
 * @param[in] ulNumPages	    Pages to erase
 * 
 * @retval::ERROR_NONE			Pages erased
 */
typedef eModuleError_t ( *fnFlashErasePages_t )( xFlashDevice_t *pxDevice, uint32_t ulStartPage, uint32_t ulNumPages );

/**@brief Erase the complete flash device
 * 
 * @param[in] pxDevice	    	Flash device
 * 
 * @retval::ERROR_NONE			Device erased
 */
typedef eModuleError_t ( *fnFlashEraseAll_t )( xFlashDevice_t *pxDevice );

/**@brief Required functionality */
struct xFlashImplementation_t
{
	fnFlashInit_t			  fnInit;
	fnFlashWake_t			  fnWake;
	fnFlashSleep_t			  fnSleep;
	fnFlashReadWriteSubpage_t fnReadSubpage;
	fnFlashReadWriteSubpage_t fnWriteSubpage;
	fnFlashReadStart_t		  fnReadStart;
	fnFlashErasePages_t		  fnErasePages;
	fnFlashEraseAll_t		  fnEraseAll;
};

/* Function Declarations ------------------------------------*/

/**@brief Initialise a flash device
 *
 * @note Must only be called once on each flash device
 * 
 * @param[in] pxDevice		                Flash device to initialise
 *
 * @retval ::ERROR_NONE 					Device was successfully initialised
 * @retval ::ERROR_INITIALISATION_FAILURE 	Initialisation failed, device is not available for use
 */
eModuleError_t eFlashInit( xFlashDevice_t *pxDevice );

/**@brief Read data from the flash device
 *
 * @note	Once the requested operation has begun, this function will block until completion
 * 			xTimeout is therefore the time to wait for the driver to become available, not an upper bound on execution time
 * 
 * @param[in] pxDevice		                Flash device to read from
 * @param[in] ullFlashAddress		        Address to read from
 * @param[in] pucData		                Buffer to read into
 * @param[in] ulLength		                Number of bytes to read
 * @param[in] xTimeout		            	Duration to wait for driver to become available
 * 
 * @retval ::ERROR_NONE 					Read completed successfully
 */
eModuleError_t eFlashRead( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint8_t *pucData, uint32_t ulLength, TickType_t xTimeout );

/**@brief Write data to the flash device
 *
 * @note	Once the requested operation has begun, this function will block until completion
 * 			xTimeout is therefore the time to wait for the driver to become available, not an upper bound on execution time
 * 
 * @param[in] pxDevice		                Flash device to read from
 * @param[in] ullFlashAddress		        Address to write to
 * @param[in] pucData		                Data to write
 * @param[in] ulLength		                Number of bytes to write
 * @param[in] xTimeout		            	Duration to wait for driver to become available
 * 
 * @retval ::ERROR_NONE 					Write completed successfully
 */
eModuleError_t eFlashWrite( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint8_t *pucData, uint32_t ulLength, TickType_t xTimeout );

/**@brief Erase data from the flash device
 * 
 * @note    ullFlashAddress and ulLength must be integer multiples of (pxDevice->xSettings.usErasePages * pxDevice->xSettings.usPageSize)
 * @note	Once the requested operation has begun, this function will block until completion
 * 			xTimeout is therefore the time to wait for the driver to become available, not an upper bound on execution time
 * 
 * @param[in] pxDevice		                Flash device to erase from
 * @param[in] ullFlashAddress		        Address to begin erase at
 * @param[in] ulLength		                Number of bytes to erase
 * @param[in] xTimeout		            	Duration to wait for driver to become available
 * 
 * @retval ::ERROR_NONE 					Erase completed successfully
 * @retval ::ERROR_INVALID_PARAMS 			ullFlashAddress or ulLength incorrectly alligned
 */
eModuleError_t eFlashErase( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint32_t ulLength, TickType_t xTimeout );

/**@brief Erase the complete flash device
 *  
 * @note	Once the requested operation has begun, this function will block until completion
 * 			xTimeout is therefore the time to wait for the driver to become available, not an upper bound on execution time
 * 
 * @param[in] pxDevice		                Flash device to erase
 * @param[in] xTimeout		            	Duration to wait for driver to become available
 * 
 * @retval ::ERROR_NONE 					Erase completed successfully
 */
eModuleError_t eFlashEraseAll( xFlashDevice_t *pxDevice, TickType_t xTimeout );

/**@brief Calculate the CRC16_CCITT over a region of memory
 * 
 * @note	Once the requested operation has begun, this function will block until completion
 * 			xTimeout is therefore the time to wait for the driver to become available, not an upper bound on execution time
 * 
 * @param[in] pxDevice		                Flash device
 * @param[in] ullFlashAddress		        Start address of the memory region
 * @param[in] ulLength		                Length of the memory region
 * @param[out] pusCrc		                Calculated CRC of the memory region
 * @param[in] xTimeout		            	Duration to wait for driver to become available
 * 
 * @retval ::ERROR_NONE 					Erase completed successfully
 */
eModuleError_t eFlashCrc( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint32_t ulLength, uint16_t *pusCrc, TickType_t xTimeout );

/**@brief Copy ROM to Flash
 * 
 * @note	Once the requested operation has begun, this function will block until completion
 * 			xTimeout is therefore the time to wait for the driver to become available, not an upper bound on execution time
 * 
 * @param[in] pxDevice		                Flash device
 * @param[in] ullFlashAddress		        Start address of flash memory to write to
 * @param[in] ulLength		                Length of ROM to copy in bytes
 * @param[in] pucRomAddress		            Start ROM address to copy
 * @param[in] xTimeout		            	Duration to wait for driver to become available
 *
 * @retval ::ERROR_NONE 					ROM store completed successfully
 */
eModuleError_t eFlashRomStore( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint32_t ulLength, uint8_t *pucRomAddress, TickType_t xTimeout );

/**@brief Copy ROM to Flash, with arbitrary bytes updated
 * 
 * pucDeltas[i] contains the length of the i'th section of ROM to copy before the next modified byte
 * pucDeltaData[i] contains the i'th new byte to write to flash
 * 
 * EXAMPLES
 * 					  @pucRomAddress
 * 							|
 * 							v
 * 			START_ROM: 	[ 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF ]
 * ----
 * 	 |   	NUM_DELTAS: 6
 *   |      DELTAS: 	[    0,    2,    3,    0,    0,    6 ]
 * 	EX1		DELTA_DATA:	[ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 ]
 * 	 |
 * 	 |		END_FLASH:	[ 0x00, 0xA1, 0xA2, 0x01, 0xA4, 0xA5, 0xA6, 0x02, 0x03, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0x05 ]
 * ----						
 * 	 |		NUM_DELTAS:	3
 *   |      DELTAS: 	[    5,    8,    0 ]
 * 	EX2		DELTA_DATA:	[ 0x00, 0x01, 0x02 ]
 *   |
 * 	 |		END_FLASH:	[ 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0x00, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0x01, 0x02 ]
 * ----						^
 * 							|
 * 					@ullFlashAddress
 * 
 * @note	Because pucDeltas is an array of uint8_t's, copying sequences larger than 255 bytes is not possible
 * 			Use eFlashRomStore for the long sequence
 * 
 * @note	Once the requested operation has begun, this function will block until completion
 * 			xTimeout is therefore the time to wait for the driver to become available, not an upper bound on execution time
 * 
 * @param[in] pxDevice		                Flash device
 * @param[in] ullFlashAddress		        Start address of flash memory to write to
 * @param[in] pucRomAddress		            ROM start address
 * @param[in] pucDeltas		            	Array of ROM lengths to copy, see examples
 * @param[in] pucDeltaData		            Array of modified bytes, see examples
 * @param[in] ucNumDeltas		            Length of pucDeltas, and pucDeltaData arrays
 * @param[in] xTimeout		            	Duration to wait for driver to become available
 *
 * @retval ::ERROR_NONE 					ROM store completed successfully
 */
eModuleError_t eFlashRomStoreDeltas( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint8_t *pucRomAddress, uint8_t *pucDeltas, uint8_t *pucDeltaData, uint8_t ucNumDeltas, TickType_t xTimeout );

/**@brief Begin a flash read operation
 * 
 * Setup device for a streaming read operation that is never intended to end
 * This function is only intended to be called in flash_to_rom.rpc implementations
 * 
 * @note	Communications interface is NOT released upon return from this function
 * 
 * @param[in] pxDevice		                Flash device
 * @param[in] ullFlashAddress		        Address to start read at
 * @param[in] xTimeout		            	Duration to wait for driver to become available
 *
 * @retval ::ERROR_NONE 					Read successfully begun
 */
eModuleError_t eFlashStartRead( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, TickType_t xTimeout );

#endif /* __CSIRO_CORE_FLASH_INTERFACE */
