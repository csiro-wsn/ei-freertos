/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: argon.h
 * Creation_Date: 20/09/2019
 * Author: Anton Schieber <anton.schieber@data61.csiro.au>
 * 			(and Jon)
 *
 * Platform defines for the argon board.
 * 
 */

/* Includes */
#include "nrf52840.h"
#include "nrf_gpio.h"

// clang-format off

/* Leds */
#define LEDS_NUMBER 3
#define LED_1		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 13 )}
#define LED_2		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 14 )}
#define LED_3		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 15 )}

#define LEDS_ACTIVE_STATE	0

/* Buttons */
#define BUTTONS_NUMBER	1
#define BUTTON_1		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 11 )}
#define BUTTON_PULL		NRF_GPIO_PIN_PULLUP

/* UART */
#define UART0				0
#define UART0_RX_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 8 )}
#define UART0_TX_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 6 )}
#define UART0_CTS_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 2 )}
#define UART0_RTS_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 1 )}

/* I2C */
#define TWIM1 				1
#define TWIM1_SDA_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 26 )}
#define TWIM1_SCL_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 27 )}

/* Wifi (through ESP32) */
#define WIFI_ENABLE_PIN  	(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 24 )}
#define ESP_BOOT_MODE_PIN 	(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 16 )}
#define ESP_HOST_WK_PIN 	(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 7 )}

#define UART1				1
#define UART1_RX_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 5 )}
#define UART1_TX_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 4 )}
#define UART1_CTS_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 7 )}
#define UART1_RTS_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 6 )}


/* SPI */
#define SPIM0				0							// SPIM0 Instance
#define SPIM0_MISO_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 14 )}	// SPI0 MISO Pin
#define SPIM0_MOSI_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 13 )}	// SPI0 MOSI Pin
#define SPIM0_SCK_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 15 )}	// SPI0 SCK Pin
#define SPIM0_SS_PIN		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 31 )}	// SPI0 SS Pin (Placeholder: Each device should have its own SS pin).

/* ADC */
#define ADC_INSTANCE 			NRF_SAADC

// clang-format on
