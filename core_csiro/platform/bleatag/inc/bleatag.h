/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef BLEATAG_H
#define BLEATAG_H

// clang-format off

/* Includes */
#include "nrf52840.h"
#include "nrf_gpio.h"

/* Leds Interface Configuration */
#define LEDS_NUMBER             3
#define LED_1                   (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 3 )}
#define LED_2                   (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 1 )}
#define LED_3                   (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 2 )}

#define LEDS_ACTIVE_STATE       1

/* Button Interface Configurations */
#define BUTTONS_ACTIVE_STATE    0
#define BUTTONS_NUMBER          1
#define BUTTON_1_GPIO           (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 15 )}
#define BUTTON_PULL		NRF_GPIO_PIN_PULLUP

/* Serial uart Interface Configuration */
#define UART0				0
#define UART_RX_GPIO           (xGpio_t){.ucPin =  NRF_GPIO_PIN_MAP( 0, 24 )}
#define UART_TX_GPIO           (xGpio_t){.ucPin =  NRF_GPIO_PIN_MAP( 0, 23 )}
#define UART_CTS_GPIO          (xGpio_t){.ucPin =  NRF_GPIO_PIN_MAP( 1, 0 )}
#define UART_RTS_GPIO          (xGpio_t){.ucPin =  NRF_GPIO_PIN_MAP( 0, 25 )}

/* MX25 External Flash Spi Interface Configuration */
#define FLASH_SPI_INSTANCE      0 // Flash Instance ID
#define FLASH_CS_GPIO           (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 14 )}
#define FLASH_SCK_GPIO          (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 15 )}
#define FLASH_MOSI_GPIO         (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 16 )}
#define FLASH_MISO_GPIO         (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 17 )}

/* BMA280 Accelerometer Spi Interface Configuration */
#define BMA280_SPI_INSTANCE     1 // ACC Instance ID
#define BMA280_CS_GPIO          (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 12 )}
#define BMA280_SCK_GPIO         (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 9 )}
#define BMA280_MOSI_GPIO        (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 11 )}
#define BMA280_MISO_GPIO        (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 8 )}
#define BMA280_INT1_GPIO        (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 14 )}
#define BMA280_INT2_GPIO        (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 13 )}
#define BMA280_ENABLE_GPIO      (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 1, 12 )}

/* ADC interface configuration */
#define ADC_INSTANCE                NRF_SAADC
#define BATTERY_VOLTAGE_GPIO        (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 2 )}
#define BATTERY_MEAS_EN_GPIO		(xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 31 )}

/* PWM interface configuration */
#define BUZZER_PWM_INSTANCE 0
#define BUZZER_PWM_GPIO         (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 8 )}
#define BUZZER_ENABLE_GPIO      (xGpio_t){.ucPin = NRF_GPIO_PIN_MAP( 0, 6 )}

#endif /* BLEATAG_H */

// clang-format on