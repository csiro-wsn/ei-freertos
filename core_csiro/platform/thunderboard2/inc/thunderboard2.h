/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: thunderboard2.h
 * Creation_Date: 10/09/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Platform file for Thunderboard 2
 * 
 */
#ifndef __CSIRO_CORE_THUNDERBOARD2
#define __CSIRO_CORE_THUNDERBOARD2
/* Includes -------------------------------------------------*/

#include "gpio.h"

/* Module Defines -------------------------------------------*/

// clang-format off

#define FLASH_MOSI_LOC          _USART_ROUTELOC0_TXLOC_LOC29
#define FLASH_MISO_LOC          _USART_ROUTELOC0_RXLOC_LOC30
#define FLASH_SCLK_LOC          _USART_ROUTELOC0_CSLOC_LOC18
#define FLASH_CS_GPIO           (xGpio_t){.ePort = gpioPortK, .ucPin = 1}

#define ACC_MOSI_LOC            _USART_ROUTELOC0_TXLOC_LOC18
#define ACC_MISO_LOC            _USART_ROUTELOC0_RXLOC_LOC18
#define ACC_SCLK_LOC            _USART_ROUTELOC0_CSLOC_LOC18
#define ACC_CS_GPIO             (xGpio_t){.ePort = gpioPortC, .ucPin = 3}
#define ACC_EN_GPIO             (xGpio_t){.ePort = gpioPortF, .ucPin = 8}
#define ACC_INT_GPIO            (xGpio_t){.ePort = gpioPortF, .ucPin = 12}

#define HALL_EFFECT_SDA_GPIO    (xGpio_t){.ePort = gpioPortB, .ucPin = 8}
#define HALL_EFFECT_SCL_GPIO    (xGpio_t){.ePort = gpioPortB, .ucPin = 9}
#define HALL_EFFECT_EN_GPIO     (xGpio_t){.ePort = gpioPortB, .ucPin = 10}
#define HALL_EFFECT_OUT_GPIO    (xGpio_t){.ePort = gpioPortB, .ucPin = 11}

#define EXTERNAL_I2C_SDA_GPIO   (xGpio_t){.ePort = gpioPortC, .ucPin = 10}
#define EXTERNAL_I2C_SCL_GPIO   (xGpio_t){.ePort = gpioPortC, .ucPin = 11}
#define EXTERNAL_I2C_SDA_LOC    I2C_ROUTELOC0_SDALOC_LOC15
#define EXTERNAL_I2C_SCL_LOC    I2C_ROUTELOC0_SCLLOC_LOC15

#define ENVIRONMENTAL_I2C_SDA_GPIO  (xGpio_t){.ePort = gpioPortC, .ucPin = 4}
#define ENVIRONMENTAL_I2C_SCL_GPIO  (xGpio_t){.ePort = gpioPortC, .ucPin = 5}
#define ENVIRONMENTAL_EN_GPIO       (xGpio_t){.ePort = gpioPortF, .ucPin = 9}
#define ENVIRONMENTAL_I2C_SDA_LOC   I2C_ROUTELOC0_SCLLOC_LOC17
#define ENVIRONMENTAL_I2C_SCL_LOC   I2C_ROUTELOC0_SDALOC_LOC17

#define AIR_QUALITY_SDA_GPIO    (xGpio_t){.ePort = gpioPortB, .ucPin = 6}
#define AIR_QUALITY_SCL_GPIO    (xGpio_t){.ePort = gpioPortB, .ucPin = 7}
#define AIR_QUALITY_SDA_LOC     I2C_ROUTELOC0_SDALOC_LOC6
#define AIR_QUALITY_SCL_LOC     I2C_ROUTELOC0_SCLLOC_LOC6
#define AIR_QUALITY_INT_GPIO    (xGpio_t){.ePort = gpioPortF, .ucPin = 13}
#define AIR_QUALITY_EN_GPIO     (xGpio_t){.ePort = gpioPortF, .ucPin = 14}
#define AIR_QUALITY_WAKE_GPIO   (xGpio_t){.ePort = gpioPortF, .ucPin = 15}

#define LIGHT_SENSOR_INT_GPIO   (xGpio_t){.ePort = gpioPortF, .ucPin = 11}

#define MICROPHONE_EN_GPIO      (xGpio_t){.ePort = gpioPortF, .ucPin = 10}
#define MICROPHONE_IN_GPIO      (xGpio_t){.ePort = gpioPortF, .ucPin = 7}

#define LED_RED_GPIO            (xGpio_t){.ePort = gpioPortI, .ucPin = 0}
#define LED_GREEN_GPIO          (xGpio_t){.ePort = gpioPortI, .ucPin = 1}
#define LED_BLUE_GPIO           (xGpio_t){.ePort = gpioPortI, .ucPin = 2}
#define LED_YELLOW_GPIO         (xGpio_t){.ePort = gpioPortI, .ucPin = 3}

#define LED_ENABLE_GPIO         (xGpio_t){.ePort = gpioPortJ, .ucPin = 14}
#define LED_ENABLE_R_GPIO       (xGpio_t){.ePort = gpioPortD, .ucPin = 11}
#define LED_ENABLE_G_GPIO       (xGpio_t){.ePort = gpioPortD, .ucPin = 12}
#define LED_ENABLE_B_GPIO       (xGpio_t){.ePort = gpioPortD, .ucPin = 13}

#define BUTTON_LEFT_GPIO        (xGpio_t){.ePort = gpioPortD, .ucPin = 14}
#define BUTTON_RIGHT_GPIO       (xGpio_t){.ePort = gpioPortD, .ucPin = 15}

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

#endif /* __CSIRO_CORE_THUNDERBOARD2 */
