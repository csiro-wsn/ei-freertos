/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth_gap.h
 * Creation_Date: 08/09/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Bluetooth stack agnostic implementation of GAP related funcionality
 * 
 * GAP covers the processes of advertising, discovery and connecting
 * 
 */
#ifndef __CSIRO_CORE_BLUETOOTH_GAP
#define __CSIRO_CORE_BLUETOOTH_GAP
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"

#include "bluetooth_types.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/**@brief GAP Scanning Configuration */
typedef struct xGapScanParameters_t
{
	eBluetoothPhy_t ePhy;			  /**< PHY to scan on */
	bool			bActiveScanning;  /**< Perform active scanning (Query advertisers for additional data) */
	uint16_t		usScanIntervalMs; /**< Period at which to swap advertising channels */
	uint16_t		usScanWindowMs;   /**< Duration to listen on each channel per period */
	fnScanRecv_t	fnCallback;		  /**< Function to call when advertising packets are observed */
} xGapScanParameters_t;

/**@brief GAP Advertisement Configuration */
typedef struct xGapAdvertiseParameters_t
{
	eBluetoothPhy_t				ePhy;				 /**< PHY to advertise on */
	eBluetoothAdvertisingType_t eType;				 /**< Connection and Scan response modes for this data */
	int8_t						cTransmitPowerDbm;   /**< TX Power */
	uint8_t						ucAdvertiseCount;	/**< Number of times to repeat advertisement data */
	uint16_t					usAdvertisePeriodMs; /**< Time between advertisement repeats */
	uint8_t						pucData[31];		 /**< Data to advertise */
	uint8_t						ucDataLen;			 /**< Bytes of data to advertise */
} xGapAdvertiseParameters_t;

/**@brief GAP Connection Configuration */
typedef struct xGapConnectionParameters_t
{
	uint16_t usConnectionInterval;  /**< Event timing desired for this connection, 1.25ms units */
	uint16_t usSlaveLatency;		/**< Number of events that can be skipped by the peripheral (GATT client) */
	uint16_t usSupervisorTimeoutMs; /**< Timeout for connection when not heard */
} xGapConnectionParameters_t;

/* Function Declarations ------------------------------------*/

/**@brief Set the parameters used for GAP Connections
 *
 * @param[in] pxConnectionParameters	Parameters to use
 * 
 * @retval ::ERROR_NONE 				Parameters updated successfully
 * @retval ::ERROR_INVALID_DATA 		Provided parameters were invalid
 */
eModuleError_t eBluetoothGapConnectionParameters( xGapConnectionParameters_t *pxConnectionParameters );

/**@brief Set the parameters used for GAP Scanning
 *
 * @param[in] pxScanParams				Parameters to use
 * 
 * @retval ::ERROR_NONE 				Parameters updated successfully
 * @retval ::ERROR_INVALID_DATA 		Provided parameters were invalid
 */
eModuleError_t eBluetoothGapScanConfigure( xGapScanParameters_t *pxScanParams );

/**@brief Start GAP scanning with the most recently provided parameters
 *
 * @param[in] ePHY						PHY layer to scan on
 * 
 * @retval ::ERROR_NONE 				Scanning started successfully
 * @retval ::ERROR_INVALID_STATE 		Stack was already scanning
 */
eModuleError_t eBluetoothGapScanStart( eBluetoothPhy_t ePHY );

/**@brief Resume the GAP scanning process
 * 
 * @note	Only applicable for stacks which pause scanning on reception of an advertisement
 *
 * @retval ::ERROR_NONE 				Scanning started successfully
 * @retval ::ERROR_INVALID_STATE 		Scanning was not paused
 */
eModuleError_t eBluetoothGapScanResume( void );

/**@brief Stop the GAP scanning process
 * 
 * @retval ::ERROR_NONE 				Scanning stopped successfully
 * @retval ::ERROR_INVALID_STATE 		Scanning was not scanning
 */
eModuleError_t eBluetoothGapScanStop( void );

/**@brief Begin an advertisement set
 *
 * @param[in] pxAdvParams				Advertisement configuration
 * 
 * @retval ::ERROR_NONE 				Parameters updated successfully
 * @retval ::ERROR_INVALID_STATE 		Stack was already advertising
 */
eModuleError_t eBluetoothGapAdvertise( xGapAdvertiseParameters_t *pxAdvParams );

/**@brief Start connecting to a remote device
 *
 * @param[in] pxConnection				Connection to initiate
 * 
 * @retval ::ERROR_NONE 				Connection initialised successfully
 * @retval ::ERROR_INVALID_STATE 		Stack was in a bad state
 */
eModuleError_t eBluetoothGapConnect( xBluetoothConnection_t *pxConnection );

/**@brief Terminate a GAP connection
 *
 * @param[in] pxConnection				Connection to disconnect
 * 
 * @retval ::ERROR_NONE 				Connection initialised successfully
 * @retval ::ERROR_INVALID_STATE 		Stack was not previously connected
 */
eModuleError_t eBluetoothGapDisconnect( xBluetoothConnection_t *pxConnection );

#endif /* __CSIRO_CORE_BLUETOOTH_GAP */
