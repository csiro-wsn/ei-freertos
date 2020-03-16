/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "test_reporting.h"

#include "log.h"

/* Private Defines ------------------------------------------*/

// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/
/* Private Variables ----------------------------------------*/

const char *pucTestStrings[] = {
	[TEST_SYSTEM]	  = "SYSTEM",
	[TEST_NVM]		   = "NVM",
	[TEST_GPS]		   = "GPS",
	[TEST_IMU]		   = "IMU",
	[TEST_TEMPERATURE] = "TEMP",
	[TEST_LR_RADIO]	= "LR_RADIO",
	[TEST_SR_RADIO]	= "SR_RADIO",
	[TEST_FLASH]	   = "FLASH",
	[TEST_SD]		   = "SD",
	[TEST_HALL_EFFECT] = "HALL_EFFECT",
	[TEST_LEDS]		   = "LEDS",
	[TEST_POWER]	   = "POWER"
};

const char *pucResultStrings[] = {
	[TEST_PASSED] = "PASSED",
	[TEST_FAILED] = "FAILED",
	[TEST_INFO]   = "INFO",
};

const char *pucSystemStrings[] = {
	[TEST_SYSTEM_PUSH_BUTTON]	 = "push_button",
	[TEST_SYSTEM_SERIAL]		  = "serial",
	[TEST_SYSTEM_MAC_ADDR]		  = "MAC",
	[TEST_SYSTEM_MANUFACTURER_ID] = "manu_id",
	[TEST_SYSTEM_ESN]			  = "ESN"
};

const char *pucNvmStrings[] = {
	[TEST_NVM_REBOOT_COUNT] = "rst_count",
};

const char *pucGpsStrings[] = {
	[TEST_GPS_VERSION] = "version",
	[TEST_GPS_TTFF]	= "ttff"
};

const char *pucImuStrings[] = {
	[TEST_IMU_WHO_AM_I]	= "who_am_i",
	[TEST_IMU_SAMPLE_RATE] = "sample_rate",
	[TEST_IMU_MAGNITUDE]   = "magnitude"
};

const char *pucTemperatureStrings[] = {
	[TEST_TEMPERATURE_ONE_SHOT]	= "one_shot",
	[TEST_TEMPERATURE_PERIODIC]	= "periodic",
	[TEST_TEMPERATURE_SAMPLE_RATE] = "sample_rate"
};

const char *pucLrRadioStrings[] = {
	[TEST_LR_RADIO_FW_VERSION] = "version",
	[TEST_LR_RADIO_ID]		   = "id",
	[TEST_LR_PACKET]		   = "packet",
	[TEST_LR_RADIO_RECV_RSSI]  = "recv_rssi",
};

const char *pucSrRadioStrings[] = {
	[TEST_SR_RADIO_CONNECTED] = "connected",
	[TEST_SR_RADIO_RECV_RSSI] = "recv_rssi",
};

const char *pucFlashStrings[] = {
	[TEST_FLASH_WHO_AM_I] = "who_am_i",
	[TEST_FLASH_WRITE]	= "write",
	[TEST_FLASH_READ]	 = "read",
	[TEST_FLASH_ERASE]	= "erase"
};

const char *pucSdStrings[] = {
	[TEST_SD_WRITE] = "write",
	[TEST_SD_READ]  = "read",
	[TEST_SD_ERASE] = "erase"
};

const char *pucHallEffectStrings[] = {
	[TEST_HALL_EFFECT_TRIGGER]	 = "hall_effect_trig",
	[TEST_HALL_EFFECT_UNTRIGGERED] = "hall_effect_untrig"
};

const char *pucLedStrings[] = {
	[TEST_LEDS_BLUE] = "leds_blue",
	[TEST_LEDS_RED]  = "leds_red"
};

const char *pucPowerStrings[] = {
	[TEST_POWER_ADC]		  = "adc",
	[TEST_POWER_MEASURE_BATT] = "batt"
};

const char **pucSubtestStrings[] = {
	[TEST_SYSTEM]	  = pucSystemStrings,
	[TEST_NVM]		   = pucNvmStrings,
	[TEST_GPS]		   = pucGpsStrings,
	[TEST_IMU]		   = pucImuStrings,
	[TEST_TEMPERATURE] = pucTemperatureStrings,
	[TEST_LR_RADIO]	= pucLrRadioStrings,
	[TEST_SR_RADIO]	= pucSrRadioStrings,
	[TEST_FLASH]	   = pucFlashStrings,
	[TEST_SD]		   = pucSdStrings,
	[TEST_HALL_EFFECT] = pucHallEffectStrings,
	[TEST_LEDS]		   = pucLedStrings,
	[TEST_POWER]	   = pucPowerStrings
};

/*-----------------------------------------------------------*/

void vReportTestResult( eReportCategory_t eTest, unsigned int ulSubTest, eReportResult_t eResult, const char *pucInfo )
{
	const char *pucTestString   = pucTestStrings[eTest];
	const char *pucResultString = pucResultStrings[eResult];
	const char *pucSubtestString;
	/* Select the correct subtest string */
	switch ( ulSubTest ) {
		case TEST_OVERALL:
			pucSubtestString = "result";
			break;
		case TEST_DRIVER:
			pucSubtestString = "driver";
			break;
		case TEST_STATUS:
			pucSubtestString = "status";
			break;
		default:
			pucSubtestString = pucSubtestStrings[eTest][ulSubTest];
	}
	/* Output result, try and keep text aligned */
	eLog( LOG_RESULT, LOG_ERROR, "%8s:%12s: %7s: %s\r\n",
		  pucTestString,
		  pucSubtestString,
		  pucResultString,
		  pucInfo );
}

/*-----------------------------------------------------------*/
