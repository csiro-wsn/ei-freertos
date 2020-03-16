/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: gpio.h
 * Creation_Date: 15/07/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Generic GPIO Interface
 * 
 */
#ifndef __CSIRO_CORE_INTERFACE_GPIO
#define __CSIRO_CORE_INTERFACE_GPIO

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"

#include "gpio_arch.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define UNUSED_GPIO 					UNUSED_GPIO_ARCH
#define ASSERT_GPIO_ASSIGNED( xGpio )	ASSERT_GPIO_ASSIGNED_ARCH( xGpio )

/* Pin States */
#define GPIO_DISABLED_NOPULL        0 /**< Disabled pin has no default state */
#define GPIO_DISABLED_PULLUP        1 /**< Disabled pin has an internal pullup */

#define GPIO_INPUT_NOFILTER         0 /**< Raw input */
#define GPIO_INPUT_FILTERED         1 /**< Glitch-suppression filter enabled (hw dependant) */

#define GPIO_INPUTPULL_PULLDOWN     0 /**< Input pin has internal pulldown enabled */
#define GPIO_INPUTPULL_PULLUP       1 /**< Input pin has internal pullup enabled */

#define GPIO_PUSHPULL_HIGH          1 /**< Output pin is driven high */
#define GPIO_PUSHPULL_LOW           0 /**< Output pin is driven low */

#define GPIO_OPENDRAIN_HIGH         1 /**< Output pin is driven high */
#define GPIO_OPENDRAIN_LOW          0 /**< Output pin is driven low */

// clang-format on
/* Type Definitions -----------------------------------------*/

/**@brief Primary GPIO type */
typedef enum {
	GPIO_DISABLED,  /**< Disabled, lowest current mode */
	GPIO_INPUT,		/**< Floating input */
	GPIO_INPUTPULL, /**< Input with a configurable push-pull resistor */
	GPIO_PUSHPULL,  /**< Pushpull output */
	GPIO_OPENDRAIN  /**< Open Drain output */
} eGpioType_t;

/**@brief Interrupt trigger edge  */
typedef enum {
	GPIO_INTERRUPT_RISING_EDGE,  /**< Interrupt triggered on a rising edge */
	GPIO_INTERRUPT_FALLING_EDGE, /**< Interrupt triggered on a falling edge */
	GPIO_INTERRUPT_BOTH_EDGE	 /**< Interrupt triggered on both edges */
} eGpioInterruptEdge_t;

typedef void ( *fnGpioInterrupt_t )( void );

/* Function Declarations ------------------------------------*/

/**@brief Initialise the GPIO driver and configure all pins as disabled
 *
 * @note Only called once on board startup
 */
void vGpioInit( void );

/**@brief	Utility function to determine if two GPIO's refer to the same physical pin
 *
 * @param[in] xGpioA		First GPIO to compare
 * @param[in] xGpioB		Second GPIO to compare
 * 
 * @retval					True if both GPIO's refer to same physical pin
 */
static inline bool bGpioEqual( xGpio_t xGpioA, xGpio_t xGpioB );

/**@brief Configure a GPIO into the provided mode
 * 
 * @note 	Overrides any previous configuration
 *
 * @param[in] xGpio			GPIO to configure
 * @param[in] eType			Mode to configure GPIO as
 * @param[in] ulParam		Mode configuration options, default states etc.
 */
void vGpioSetup( xGpio_t xGpio, eGpioType_t eType, uint32_t ulParam );

/**@brief Set a GPIO to the provided value
 *
 * @note	Only has an effect if the GPIO was previously configured as an output
 * 
 * @param[in] xGpio			GPIO to update
 * @param[in] bValue		True to output logic level high, false for logic level low
 */
void vGpioWrite( xGpio_t xGpio, bool bValue );

/**@brief Set a GPIO to logic level high
 *
 * @note	Only has an effect if the GPIO was previously configured as an output
 * 
 * @param[in] xGpio			GPIO to update
 */
void vGpioSet( xGpio_t xGpio );

/**@brief Set a GPIO to logic level low
 *
 * @note	Only has an effect if the GPIO was previously configured as an output
 * 
 * @param[in] xGpio			GPIO to update
 */
void vGpioClear( xGpio_t xGpio );

/**@brief Set a GPIO to the inverse of its current value
 *
 * @note	Only has an effect if the GPIO was previously configured as an output
 * 
 * @param[in] xGpio			GPIO to update
 */
void vGpioToggle( xGpio_t xGpio );

/**@brief Read the logic level of a GPIO
 *
 * @note	Only valid if the GPIO was previously configured as an input
 * 
 * @param[in] xGpio			GPIO to update
 * 
 * @retval					Logic level of GPIO
 */
bool bGpioRead( xGpio_t xGpio );

/**@brief Configure the interrupt associated with a GPIO
 *
 * @note 	Overrides any previous configuration
 * @note	Interrupts will only be triggered if the GPIO is configured as an input
 * 
 * @param[in] xGpio				GPIO to update
 * @param[in] bEnable			True to enable the interrupt, false to disable
 * @param[in] eInterruptEdge	If interrupt enabled, which edge to trigger on
 * @param[in] fnCallback		If interrupt enabled, the function to call on trigger
 * 
 * @retval ::ERROR_NONE 					Interrupt successfully configured
 * @retval ::ERROR_UNAVAILABLE_RESOURCE 	No free interrupt lines to use
 */
eModuleError_t eGpioConfigureInterrupt( xGpio_t xGpio, bool bEnable, eGpioInterruptEdge_t eInterruptEdge, fnGpioInterrupt_t fnCallback );

#endif /* __CSIRO_CORE_INTERFACE_GPIO */
