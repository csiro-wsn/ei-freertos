/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "temp.h"

#include "adc.h"
#include "adc_arch.h"

#include "nrf_saadc.h"
#include "nrfx_saadc.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

#define TEMP_RECALIBRATION_THRESHOLD 10000 // 10 degrees

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

nrf_saadc_input_t	 prvGpioToAnalogPortMapping( xGpio_t xGpio );
uint8_t				  prvGpioToAnalogChannelMapping( xGpio_t xGpio );
nrf_saadc_gain_t	  prvAdcGainMapping( eAdcReferenceVoltage_t eAdcReferenceVoltage );
nrf_saadc_reference_t prvAdcReferenceVoltageMapping( eAdcReferenceVoltage_t eAdcReferenceVoltage );

void vAdcInterruptHandler( nrfx_saadc_evt_t const *pxEvent );

/* Private Variables ----------------------------------------*/

STATIC_SEMAPHORE_STRUCTURES( xSamplingDoneSemaphore );

/* Functions ------------------------------------------------*/

/**
 * Initialise the ADC.
 *
 * This function is called once on startup. It sets the ADC registers into a sensible mode.
 **/
void vAdcInit( xAdcModule_t *pxAdc )
{
	/* Create mutexes for access control */
	pxAdc->xModuleAvailableHandle = xSemaphoreCreateBinaryStatic( &( pxAdc->xModuleAvailableStorage ) );
	xSemaphoreGive( pxAdc->xModuleAvailableHandle );

	STATIC_SEMAPHORE_CREATE_BINARY( xSamplingDoneSemaphore );
}

/*-----------------------------------------------------------*/

/**
 * Calculate a single sample with the ADC and return it.
 *
 * This function sets everything up, and busy waits for the ADC to calculate
 * a sample and return it. It takes ~45uS for this function to complete. It
 * would be super fantastic if this could be all be implemented without busy
 * waiting, but none of their libraries implement it, and it's a good couple
 * days work for like 50uS of idle time. Most likely not worth it.
 * 
 * Note: Sampling anything over VCC or under VDD will crash the board.
 **/

uint32_t ulAdcSample( xAdcModule_t *pxAdc, xGpio_t xGpio, eAdcResolution_t eResolution, eAdcReferenceVoltage_t eReferenceVoltage )
{
	nrf_saadc_value_t pxAdcValue[2];

	/**
	 * Oversample to reduce noise in measurements. Each sample is the average
	 * of 2^4 samples.
	 **/
	nrfx_saadc_config_t xAdcInit = NRFX_SAADC_DEFAULT_CONFIG;
	xAdcInit.oversample			 = 4;

	/**
	 * Use the default settings, with relevent reference and gain.
	 * Enable burst so that nrfx_saadc_sample() only needs to be called once
	 * for each final measurement, and not once for each sample. (Stops us
	 * from getting tonnes of interrupts we don't care about).
	 **/
	nrf_saadc_channel_config_t xAdcChannelInit = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE( prvGpioToAnalogPortMapping( xGpio ) );
	xAdcChannelInit.burst					   = NRF_SAADC_BURST_ENABLED;
	xAdcChannelInit.reference				   = prvAdcReferenceVoltageMapping( eReferenceVoltage );
	xAdcChannelInit.gain					   = prvAdcGainMapping( eReferenceVoltage );

	/**
	 * Take a semaphore so that multiple threads aren't trying to all get the
	 * battery voltage at the same time. If it takes over a second, something
	 * is terribly wrong, so reboot.
	 **/
	configASSERT( xSemaphoreTake( pxAdc->xModuleAvailableHandle, pdMS_TO_TICKS( 1000 ) ) == pdTRUE );

	/* Initialise the SAADC module. */
	configASSERT( nrfx_saadc_init( &xAdcInit, vAdcInterruptHandler ) == NRFX_SUCCESS );

	/* Initialise the SAADC channel. */
	configASSERT( nrfx_saadc_channel_init( prvGpioToAnalogChannelMapping( xGpio ), &xAdcChannelInit ) == NRFX_SUCCESS );

	/* Set the resolution of the measurement */
	nrf_saadc_resolution_set( eResolution );

	/**
	 * Register the buffer to write data into when sampling. Using these
	 * buffers is required for sampling the ADC without busy waiting for a
	 * conversion to finish. The size of the buffer is in words.
	 **/
	nrfx_saadc_buffer_convert( pxAdcValue, 1 );

	/* Trigger an ADC sample */
	nrfx_saadc_sample();

	/**
	 * Take a semaphore to wait for the sampling to finish. A single sample
	 * should never take longer than a second, so if it does, reboot because
	 * something is probably wrong.
	 **/
	configASSERT( xSemaphoreTake( xSamplingDoneSemaphore, pdMS_TO_TICKS( 1000 ) ) == pdTRUE );

	/**
	 * Once the conversion is finished, give the semaphore back because other
	 * tasks can now sample the ADC.
	 **/
	xSemaphoreGive( pxAdc->xModuleAvailableHandle );

	/**
	 * Result can be negative, because nrf_saadc_value_t is a sneaky signed
	 * variable. This doesn't really make sense in the way we most commonly use
	 * the ADC, so let's just change negatives to zero.
	 **/
	if ( pxAdcValue[0] < 0 ) {
		pxAdcValue[0] = 0;
	}

	return (uint32_t) pxAdcValue[0];
}

/*-----------------------------------------------------------*/

/**
 * This function recalibrates the ADC in the nRF52. It only reclibrates if the
 * change in temperature since last calibration is greater than 10 degrees.
 * 
 * When reclibrating it ensures nobody is using ADC module the function before
 * we attempt to use it, then waits for recalibration to complete before
 * finishing.
 **/
eModuleError_t eAdcRecalibrate( xAdcModule_t *pxAdc )
{
	int32_t lCurrentTemp, lTempDifference;

	eTempMeasureMilliDegrees( &lCurrentTemp );

	lTempDifference = lCurrentTemp - pxAdc->xPlatform.lLastCalibratedTemperatureMilliDegrees;

	if ( ( lTempDifference < -TEMP_RECALIBRATION_THRESHOLD ) || ( lTempDifference > TEMP_RECALIBRATION_THRESHOLD ) ) {

		/* Save the current temperature as the last time the ADC was calibrated */
		pxAdc->xPlatform.lLastCalibratedTemperatureMilliDegrees = lCurrentTemp;

		configASSERT( xSemaphoreTake( pxAdc->xModuleAvailableHandle, pdMS_TO_TICKS( 1000 ) ) == pdTRUE );
		configASSERT( nrfx_saadc_calibrate_offset() == NRFX_SUCCESS );

		/* It should take ~1ms to finishing calibration. */
		configASSERT( xSemaphoreTake( xSamplingDoneSemaphore, pdMS_TO_TICKS( 1000 ) ) == pdTRUE );
		xSemaphoreGive( pxAdc->xModuleAvailableHandle );
	}

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

uint8_t prvGpioToAnalogChannelMapping( xGpio_t xGpio )
{
	/**
	 * This function maps an analog input to an analog channel, of which there are 8.
	 * AIN0-AIN7 are mapped to channels of the same index, while VDD input is always 
	 * mapped to the last channel. The implication of catering for having VDD as a 
	 * valid ADC input is VDD and AIN7 cannot be sampled at the same time.
	 **/
	uint8_t ucChannel = 0;
	if ( xGpio.ucPin == 0 ) {
		ucChannel = NRF_SAADC_CHANNEL_COUNT - 1;
	}
	else {
		ucChannel = (uint8_t) prvGpioToAnalogPortMapping( xGpio ) - 1;
	}
	return ucChannel;
}
/*-----------------------------------------------------------*/

nrf_saadc_input_t prvGpioToAnalogPortMapping( xGpio_t xGpio )
{
	/**
	 * This function only returns correctly if an analog pin is sampled. If
	 * anything else is sampled, then it will get stuck in an assert and wait
	 * there for the author to realize their mistake.
	 * An invalid GPIO is being used to cater for VDD input as VDD pins are 
	 * not GPIO. 
	 **/
	switch ( xGpio.ucPin ) {
		case 0:
			return NRF_SAADC_INPUT_VDD;
		case 2:
			return NRF_SAADC_INPUT_AIN0;
		case 3:
			return NRF_SAADC_INPUT_AIN1;
		case 4:
			return NRF_SAADC_INPUT_AIN2;
		case 5:
			return NRF_SAADC_INPUT_AIN3;
		case 28:
			return NRF_SAADC_INPUT_AIN4;
		case 29:
			return NRF_SAADC_INPUT_AIN5;
		case 30:
			return NRF_SAADC_INPUT_AIN6;
		case 31:
			return NRF_SAADC_INPUT_AIN7;
		default:
			/* We should never get here */
			configASSERT( 0 );
			return NRF_SAADC_INPUT_DISABLED;
	}
}

/*-----------------------------------------------------------*/

/**
 * The nrf52 has a single internal 0.6V reference voltage or an external 
 * VDD/4 reference, and has an input gain stage which can be set by the user. 
 * This function returns the appropriate input gain to achieve the specified 
 * reference voltage. 
 **/
nrf_saadc_gain_t prvAdcGainMapping( eAdcReferenceVoltage_t eAdcReferenceVoltage )
{
	switch ( eAdcReferenceVoltage ) {
		case ADC_REFERENCE_VOLTAGE_0V6:
			return NRF_SAADC_GAIN1;
		case ADC_REFERENCE_VOLTAGE_1V2:
			return NRF_SAADC_GAIN1_2;
		case ADC_REFERENCE_VOLTAGE_1V8:
			return NRF_SAADC_GAIN1_3;
		case ADC_REFERENCE_VOLTAGE_2V4:
			return NRF_SAADC_GAIN1_4;
		case ADC_REFERENCE_VOLTAGE_3V:
			return NRF_SAADC_GAIN1_5;
		case ADC_REFERENCE_VOLTAGE_3V6:
			return NRF_SAADC_GAIN1_6;
		case ADC_REFERENCE_VOLTAGE_VDD:
			return NRF_SAADC_GAIN1_4;
		default:
			/**
			 * If you get here, you are trying to use an unimplemented voltage
			 * scaling factor. Implement it yourself.
			 **/
			configASSERT( 0 );
			return NRF_SAADC_GAIN1_6;
	};
}

/*-----------------------------------------------------------*/

/**
 * Function returns which reference voltage to use, (internal 0.6 or external Vdd/4) based 
 * on the desired reference voltage. 
 **/
nrf_saadc_reference_t prvAdcReferenceVoltageMapping( eAdcReferenceVoltage_t eAdcReferenceVoltage )
{
	if ( eAdcReferenceVoltage == ADC_REFERENCE_VOLTAGE_VDD ) {
		return NRF_SAADC_REFERENCE_VDD4;
	}
	return NRF_SAADC_REFERENCE_INTERNAL;
}

/*-----------------------------------------------------------*/

void vAdcInterruptHandler( nrfx_saadc_evt_t const *pxEvent )
{
	BaseType_t *pxHigherPriorityTaskWoken = pdFALSE;

	/**
	 * If the interrupt handler is fired on a EVT_DONE event, we want to
	 * uninitialise the saadc module. Keeping it initialised keeps EasyDMA
	 * enabled which draws ~ 1mA of current.
	 * 
	 * Reference: https://github.com/NordicPlayground/nRF52-ADC-examples/blob/11.0.0/saadc_low_power/main.c
	 **/
	if ( pxEvent->type == NRFX_SAADC_EVT_DONE ) {
		nrfx_saadc_uninit();
		xSemaphoreGiveFromISR( xSamplingDoneSemaphore, pxHigherPriorityTaskWoken );
	}
	else if ( pxEvent->type == NRFX_SAADC_EVT_CALIBRATEDONE ) {
		xSemaphoreGiveFromISR( xSamplingDoneSemaphore, pxHigherPriorityTaskWoken );
	}
}

/*-----------------------------------------------------------*/
