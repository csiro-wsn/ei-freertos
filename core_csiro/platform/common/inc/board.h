/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: board.h
 * Creation_Date: 2/6/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Board Interface which all platforms must implement to work with existing applications
 * 
 */

#ifndef __CORE_CSIRO_PLATFORM_COMMON_BOARD
#define __CORE_CSIRO_PLATFORM_COMMON_BOARD

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"

#include "adc.h"
#include "gpio.h"
#include "uart.h"

#include "device_constants.h"
#include "flash_interface.h"
#include "serial_interface.h"

#include "device_nvm.h"

#include "board_arch.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

extern xDeviceConstants_t xDeviceConstants;

extern xFlashDevice_t *const  pxOnboardFlash;
extern xSerialModule_t *const pxSerialOutput;

/* Function Declarations ------------------------------------*/

/**@brief Retrieve the current system uptime
 *
 * @note This function is implemented 
 * 
 * @retval		System Uptime in seconds
 */
uint32_t ulApplicationUptime( void );

/**
  @defgroup Available override functions
  @{

  @brief Functions which CAN be overriden on an application or platform level
 */

/**@brief Set default log levels
 *
 * 	Uses this function to initialise logs before any initialisation occurs
 * 
 * 	@note This function is called once before initialisation of any hardware
 * 	@note No system interfaces are available in this function
 */
void vApplicationSetLogLevels( void );

/**@brief Startup callback
 *
 * 	Uses this function to initialise application specific tasks / functionality
 * 
 * 	@note This function is called once only upon complete initialisation of the hardware
 */
void vApplicationStartupCallback( void );

/**@brief Once a second tick callback
 *
 * 	This function is called once per second from the heartbeat thread.
 * 	The complete execution time of this function MUST be less than one second.
 */
void vApplicationTickCallback( uint32_t ulUptime );

/**@brief Block initialisation of peripherals until voltage condition
 *
 * 	An application can block in this function until an acceptable battery voltage is reached.
 *  This function is called before any external peripherals / power domains are enabled.
 * 
 *	This function is intended to help recovery from Out-Of-Power condition on boards that seperate
 *	external peripherals onto an external power domain by minimising current consumption until the
 *  battery can support the requirements on initialisation.
 */
void vApplicationWaitUntilValidVoltage( void );

/**@brief Block initialisation of peripherals until arbitrary condition
 *
 * 	An application can block in this function until some condition is met.
 *  The only peripherals available to use in this function are those that are always enabled on the platform hardware.
 * 
 *	This function is intended to increase the battery life of a device in a shipping mode by minimising
 *  the current consumption until it is activated.
 */
void vApplicationWaitUntilActivated( void );

/**@brief Provide an alternate serial byte handler
 *
 *  If the default Unified Serial Comms handler is not desired, overwrite it with this function
 * 
 * @retval		Alternate serial byte handler
 */
fnSerialByteHandler_t fnBoardSerialHandler( void );

/**
  @}
*/

/**
  @defgroup Platform required functions
  @{

  @brief Functions which MUST be implemented by each platform for correct operation
 */

/**@brief Configure core CPU functionality
 *
 * 	Enable internal power domains
 * 	Setup external oscillators
 * 	Configure chip errata
 * 
 * 	@note This function is called before the FreeRTOS kernel is started
 */
void vBoardSetupCore( void );

/**@brief Initialise system services and peripherals
 *
 *	Configures communications interfaces (UART, SPI, I2C, etc)
 * 	Configures Real-Time clocks, watchdogs and other board services
 * 	
 * 	Upon exiting from this function, the board is in its absolute lowest power state
 * 
 * 	@note This function is called from the main heartbeat task
 */
void vBoardInit( void );

/**@brief Tickle watchdog timer
 *
 * @note This function is called once a second from the main heartbeat task
 */
void vBoardWatchdogPeriodic( void );

/**@brief Request the board to enable the provide peripheral
 * 
 *  Upon a successful call, the peripheral can now be used as if it were always connected.
 * 	This function is intended to abstract away board specific circuitry not directly connected to the peripheral.
 * 		Examples would be SPI bus switches, RF switches, dedicated power supplies, etc
 * 
 * @param[in]  ePeripheral		Peripheral to enable
 * @param[out] bPowerApplied	Set to true if power was applied as a result of this function call
 * 								Set to false if function failed, or peripheral was already powered
 * 								This parameter can be NULL to ignore the transition mode
 * @param[in]  xTimeout			How long to wait while attempting to enable the peripheral
 * 
 * @retval::ERROR_NONE			Peripheral was successfully enabled and is ready for use
 * @retval::ERROR_TIMEOUT		Peripheral was not enabled due to resource unavailability
 */
eModuleError_t eBoardEnablePeripheral( ePeripheral_t ePeripheral, bool *pbPowerApplied, TickType_t xTimeout );

/**@brief Notify the board that the provided peripheral can be disabled
 * 
 * @param[in]  ePeripheral		Peripheral to disable
 */
void vBoardDisablePeripheral( ePeripheral_t ePeripheral );

/**@brief Read the external battery voltage
 * 
 * @note Does not enable any required external circuitry
 * 
 * @retval	Battery voltage in MilliVolts
 */
uint32_t ulBoardBatteryVoltageMV( void );

/**@brief Read the external battery charge current
 * 
 * @note Does not enable any required external circuitry
 * 
 * @retval	Charge current in MicroAmperes
 */
uint32_t ulBoardBatteryChargeUA( void );

/* Read the analog voltage on an arbitrary GPIO */
/* GPIO must already have been configured as an input */
uint32_t ulBoardAdcSample( xGpio_t xGpio, eAdcResolution_t eResolution, eAdcReferenceVoltage_t eReferenceVoltage );

/**@brief Recalibrate the ADC used in ulBoardAdcSample
 * 
 * @retval::ERROR_NONE			ADC was recalibrated successfully
 * @retval::ERROR_TIMEOUT		ADC was in use and could not be recalibrated
 */
eModuleError_t eBoardAdcRecalibrate( void );

/**@brief Modify board's configurations upon modifications of NVM values
 * 
 * @param[in]  eKey		      NVM key for which value has changed recently
 * 
 * @retval::ERROR_NONE      Board reconfigured successfully	
 */
eModuleError_t eBoardReconfigureFromNvm( eNvmKey_t eKey );

/**
  @}
*/

#endif /* __CORE_CSIRO_PLATFORM_COMMON_BOARD */
