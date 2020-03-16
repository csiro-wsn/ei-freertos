/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: test_reporting.h
 * Creation_Date: 01/09/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Test reporting functions
 * 
 */
#ifndef __CSIRO_CORE_TASKS_TEST_REPORTING
#define __CSIRO_CORE_TASKS_TEST_REPORTING
/* Includes -------------------------------------------------*/

#include <stdbool.h>

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eReportCategory_t {
	TEST_SYSTEM,
	TEST_NVM,
	TEST_GPS,
	TEST_IMU,
	TEST_TEMPERATURE,
	TEST_LR_RADIO,
	TEST_SR_RADIO,
	TEST_FLASH,
	TEST_SD,
	TEST_LEDS,
	TEST_HALL_EFFECT,
	TEST_POWER
} eReportCategory_t;

typedef enum eReportResult_t {
	TEST_PASSED,
	TEST_FAILED,
	TEST_INFO
} eReportResult_t;

enum {
	TEST_OVERALL = 255,
	TEST_DRIVER  = 254,
	TEST_STATUS  = 253,
};

enum {
	TEST_SYSTEM_SERIAL,
	TEST_SYSTEM_ADC,
	TEST_SYSTEM_PUSH_BUTTON,
	TEST_SYSTEM_MAC_ADDR,
	TEST_SYSTEM_MANUFACTURER_ID,
	TEST_SYSTEM_ESN
};

enum {
	TEST_NVM_REBOOT_COUNT,
};

enum {
	TEST_GPS_VERSION,
	TEST_GPS_TTFF
};

enum {
	TEST_IMU_WHO_AM_I,
	TEST_IMU_SAMPLE_RATE,
	TEST_IMU_MAGNITUDE
};

enum {
	TEST_TEMPERATURE_ONE_SHOT,
	TEST_TEMPERATURE_PERIODIC,
	TEST_TEMPERATURE_SAMPLE_RATE
};

enum {
	TEST_LR_RADIO_FW_VERSION,
	TEST_LR_RADIO_ID,
	TEST_LR_PACKET,
	TEST_LR_RADIO_RECV_RSSI
};

enum {
	TEST_SR_RADIO_CONNECTED,
	TEST_SR_RADIO_RECV_RSSI
};

enum {
	TEST_FLASH_WHO_AM_I,
	TEST_FLASH_WRITE,
	TEST_FLASH_READ,
	TEST_FLASH_ERASE
};

enum {
	TEST_SD_WRITE,
	TEST_SD_READ,
	TEST_SD_ERASE
};

enum {
	TEST_HALL_EFFECT_TRIGGER,
	TEST_HALL_EFFECT_UNTRIGGERED
};

enum {
	TEST_LEDS_BLUE,
	TEST_LEDS_RED
};

enum {
	TEST_POWER_ADC,
	TEST_POWER_MEASURE_BATT
};

/* Function Declarations ------------------------------------*/

void vReportTestResult( eReportCategory_t eTest, unsigned int ulSubTest, eReportResult_t eResult, const char *pucInfo );

/**
 * Function expected to be implemented by the test application
 */
void vReportTestComplete( bool bTestSucceeded );

#endif /* __CSIRO_CORE_TASKS_TEST_REPORTING */
