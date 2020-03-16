/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: core_types.h
 * Creation_Date: 04/08/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Core data types that are utilised throughout the repository
 * 
 */
#ifndef __CSIRO_CORE_CORE_TYPES
#define __CSIRO_CORE_CORE_TYPES
/* Includes -------------------------------------------------*/

/* Type Definitions -----------------------------------------*/

typedef enum eModuleError_t {
	ERROR_NONE = 0,
	ERROR_TIMEOUT,
	ERROR_GENERIC,
	ERROR_NO_CHANGE,
	ERROR_INITIALISATION_FAILURE,
	ERROR_UNAVAILABLE_RESOURCE,
	ERROR_INVALID_DATA,
	ERROR_INVALID_STATE,
	ERROR_INVALID_CRC,
	ERROR_INVALID_ADDRESS,
	ERROR_INVALID_LOGGER,
	ERROR_INVALID_LOG_LEVEL,
	ERROR_INVALID_TIME,
	ERROR_BLUETOOTH_NOT_CONNECTED,
	ERROR_BLUETOOTH_NOT_SUBSCRIBED,
	ERROR_FLASH_OPERATION_FAIL,
	ERROR_COMMAND_NOT_ACCEPTED,
	ERROR_DATA_TOO_LARGE,
	ERROR_DEVICE_FULL,
	ERROR_DEVICE_FAIL,
	ERROR_NO_MATCH,
	ERROR_NO_ACKNOWLEDGEMENT,
	ERROR_PARTIAL_COMMAND,
	ERROR_DEFAULT_CASE,
	ERROR_RPC_INVALID_PARAMS = 252,
	ERROR_RPC_UNREACHABLE	= 253,
	ERROR_RPC_NOSUCHCMD		 = 254,
	ERROR_RPC_GENERIC		 = 255
} eModuleError_t;

/**@brief Hardware Peripherals Declaration 
 * 
 * Indexing of different hardware peripheral types
 * 
 * @note 	Assumption that all functions utilising this type will 
 * 			refer to all instances of a type at once
 */
typedef enum ePeripheral_t {
	PERIPHERAL_NONE,				 /**< No external peripheral */
	PERIPHERAL_IMU,					 /**< Inertial Measurement Unit (IMU) */
	PERIPHERAL_GPS,					 /**< Global Positioning System */
	PERIPHERAL_ENVIRONMENTAL_SENSOR, /**< Environmental sensors (Temperature, Pressure, Humidity) */
	PERIPHERAL_BATTERY_MONITORING,   /**< Measuring battery voltage and charge current */
	PERIPHERAL_ONBOARD_FLASH,		 /**< Onboard Flash Memory */
	PERIPHERAL_EXTERNAL_FLASH,		 /**< External Flash Memory (SD Card) */
	PERIPHERAL_SATELLITE_COMMS,		 /**< Satellite Communication Systems */
	PERIPHERAL_LONG_RANGE_COMMS		 /**< Terrestrial Long-Range Radio (LoRa, Sigfox, Zigbee etc) */
} ePeripheral_t;

#endif /* __CSIRO_CORE_CORE_TYPES */
