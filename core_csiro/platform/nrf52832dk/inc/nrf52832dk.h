/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: nrf52840dk.h
 * Creation_Date: 25/02/2019
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 * 			(And Jordan, and Jessie, and Jon, and Luke)
 *
 * Platform defines for the nrf52840dk board.
 * 
 */

/* Includes */
#include "nrf52.h"
#include "nrf_gpio.h"

// clang-format off

/* Leds */
#define LEDS_NUMBER 4
#define LED_1		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 17 )}
#define LED_2		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 18 )}
#define LED_3		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 19 )}
#define LED_4		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 20 )}

#define LEDS_ACTIVE_STATE	0

/* Buttons */
#define BUTTONS_NUMBER	4
#define BUTTON_1		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 13 )}
#define BUTTON_2		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 14 )}
#define BUTTON_3		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 15 )}
#define BUTTON_4		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 16 )}
#define BUTTON_PULL		NRF_GPIO_PIN_PULLUP

/* UART */
#define UART0_RX_PIN	(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 8 )}
#define UART0_TX_PIN	(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 6 )}
#define UART0_CTS_PIN	(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 7 )}
#define UART0_RTS_PIN	(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 5 )}

/* I2C */
#define TWIM1 			1
#define TWIM1_SDA_PIN	(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 1 )}
#define TWIM1_SCL_PIN	(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 2 )}

/* SPI */
#define SPIM0				0							// SPIM0 Instance
#define SPIM0_MISO_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 28 )}	// SPI0 MISO Pin
#define SPIM0_MOSI_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 25 )}	// SPI0 MOSI Pin
#define SPIM0_SCK_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 29 )}	// SPI0 SCK Pin
#define SPIM0_SS_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 12 )}	// SPI0 SS Pin (Placeholder: Each device should have its own SS pin).

/* ADC */
#define ADC_INSTANCE 			NRF_SAADC
#define ADC_CHANNEL_DEFAULT 	0
#define BATTERY_VOLTAGE_PIN 	4

// clang-format on
