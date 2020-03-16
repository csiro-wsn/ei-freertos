/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "adc.h"
#include "adc_arch.h"
#include "gpio.h"
#include "log.h"

#include "em_adc.h"
#include "em_cmu.h"

/* Private Defines ------------------------------------------*/

#define ADC_CLOCK 16000000 // 16MHz is the maximum ADC_CLOCK speed.

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

ADC_PosSel_TypeDef eGpioToAportChannelMapping( xGpio_t xGpio );
ADC_Res_TypeDef	eAdcResolutionGet( eAdcResolution_t eAdcResolution );
ADC_Ref_TypeDef	eAdcReferenceVoltageGet( eAdcReferenceVoltage_t eAdcReferenceVoltage );

/* Private Variables ----------------------------------------*/

STATIC_SEMAPHORE_STRUCTURES( pxAdcAccess );

/* Functions ------------------------------------------------*/

/* Initialise the ADC.
 * 
 * This function is called once on startup. It configures turns on the ADC
 * clock, (which is required) sets the ADC registers into a sensible mode,
 * and then turns the clock back off to save power.
 */
void vAdcInit( xAdcModule_t *pxAdcModule )
{
	CMU_ClockEnable( cmuClock_ADC0, true ); // Turn ADC clock on.

	ADC_Init_TypeDef xAdcInitStruct = ADC_INIT_DEFAULT;

	// Create Mutex for access control
	STATIC_SEMAPHORE_CREATE_MUTEX( pxAdcAccess );

	xAdcInitStruct.ovsRateSel = adcOvsRateSel16;	   // This is the oversampling rate. Whether or not it is used depends on the resolution field in sample.
	xAdcInitStruct.timebase   = ADC_TimebaseCalc( 0 ); // Using 0 for HFPERCLK causes ADC_TimebaseCalc to get the value from the Silicon Labs drivers. It doesn't use zero.
	xAdcInitStruct.prescale   = ADC_PrescaleCalc( ADC_CLOCK, 0 );
	ADC_Init( pxAdcModule->xPlatform.pxADC, &xAdcInitStruct );

	CMU_ClockEnable( cmuClock_ADC0, false ); // Turn ADC clock off.
}

/*-----------------------------------------------------------*/

/* Take a single sample with the ADC and returns it.
 *
 * This function gets a sample from the ADC. It turns the clock on,
 * sets the ADC_InitSingle_Struct according to the simplified settings we
 * provide, starts the ADC conversion, busy waits for it to finish,
 * gets the data and returns. Since the ADC process is so short,
 * we can busy wait without any huge power issues.
 */
uint32_t ulAdcSample( xAdcModule_t *pxAdcModule, xGpio_t xGpio, eAdcResolution_t eResolution, eAdcReferenceVoltage_t eReferenceVoltage )
{
	uint32_t			   ulSample				= 0;
	ADC_InitSingle_TypeDef xAdcInitSingleStruct = ADC_INITSINGLE_DEFAULT;

	xSemaphoreTake( pxAdcAccess, portMAX_DELAY );

	CMU_ClockEnable( cmuClock_ADC0, true ); // Turn clock on.

	xAdcInitSingleStruct.posSel		= eGpioToAportChannelMapping( xGpio );
	xAdcInitSingleStruct.reference  = eAdcReferenceVoltageGet( eReferenceVoltage );
	xAdcInitSingleStruct.acqTime	= adcAcqTime4;
	xAdcInitSingleStruct.resolution = eAdcResolutionGet( eResolution );
	ADC_InitSingle( pxAdcModule->xPlatform.pxADC, &xAdcInitSingleStruct );

	/* Start a conversion and busy wait for it to finish. */
	ADC_Start( pxAdcModule->xPlatform.pxADC, adcStartSingle );
	while ( pxAdcModule->xPlatform.pxADC->STATUS & ADC_STATUS_SINGLEACT ) {
		;
	}

	ulSample = ADC_DataSingleGet( pxAdcModule->xPlatform.pxADC ); // Get ADC result.

	CMU_ClockEnable( cmuClock_ADC0, false ); // Turn clock off.

	xSemaphoreGive( pxAdcAccess );

	return ulSample;
}

/*-----------------------------------------------------------*/

/**
 * The calibration process of the ADC in the EFR32 chip is complicated and
 * described in the reference manual. Since we will no longer be using this
 * chip, I am not going to implement it, but if you chose to, good luck. :)
 **/
eModuleError_t eAdcRecalibrate( xAdcModule_t *pxAdc )
{
	UNUSED( pxAdc );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/* A handy dandy function for converting simple pin/port macros to the
 * infrequenctly used APORT/Channel macros used for analog functionality on
 * the EFR32. Stops the programmer from having to look at their chips datasheet
 * every time they want to read an ADC value.
 */
ADC_PosSel_TypeDef eGpioToAportChannelMapping( xGpio_t xGpio )
{
	if ( ( xGpio.ePort == gpioPortA ) && ( xGpio.ucPin == 6 ) ) {
		return adcPosSelAPORT3XCH14;
	}
	else if ( ( xGpio.ePort == gpioPortB ) && ( xGpio.ucPin == 11 ) ) {
		return adcPosSelAPORT4XCH27;
	}
	else if ( ( xGpio.ePort == gpioPortB ) && ( xGpio.ucPin == 12 ) ) {
		return adcPosSelAPORT3XCH28;
	}

	/**
	 * Congratulations, if you reach this assert it is because nobody has ever
     * used that pin from an EFR for reading ADC values before. Find the
     * APORT/Channel mapping on the chip data sheet. (Search: "APORT"), and add
     * it into this list.
     * 
     * NOTE: There will be two definitions of the same pin/port. Use the one on
     * APORT "X", not "Y". And you're also going to have to add the enum to
     * em_adc.h in mocks, or all the tests will break.
     */

	eLog( LOG_ADC, LOG_ERROR, "ADC Error: Bad GPIO.\r\n" );
	configASSERT( 0 );

	return adcPosSelDEFAULT;
}

/*-----------------------------------------------------------*/

/**
 * A switch that takes a simplified resolution enum, and spits out the
 * complicated one required by the actual structs from em_adc.h. The purpose
 * of this function is to drop 90% of the complexity from em_adc.h, and only
 * expose the simplest resolutions.
 */
ADC_Res_TypeDef eAdcResolutionGet( eAdcResolution_t eAdcResolution )
{
	switch ( eAdcResolution ) {
		case ADC_RESOLUTION_12BIT:
			return adcRes12Bit;
		case ADC_RESOLUTION_16BIT:
			return adcResOVS;
		default:
			return adcRes12Bit;
	};
}

/*-----------------------------------------------------------*/

/**
 * A switch that takes a simplifier reference voltage enum, and spits out the
 * complicated one required by the actual structs from em_adc.h. The purpose
 * of this function is to drop 90% of the complexity from em_adc.h, by only
 * expose the simplest reference voltage settings.
 */
ADC_Ref_TypeDef eAdcReferenceVoltageGet( eAdcReferenceVoltage_t eAdcReferenceVoltage )
{
	switch ( eAdcReferenceVoltage ) {
		case ADC_REFERENCE_VOLTAGE_1V25:
			return adcRef1V25;
		case ADC_REFERENCE_VOLTAGE_2V5:
			return adcRef2V5;
		case ADC_REFERENCE_VOLTAGE_5V:
			return adcRef5V;
		case ADC_REFERENCE_VOLTAGE_VDD:
			return adcRefVDD;
		default:
			return adcRef2V5;
	};
}

/*-----------------------------------------------------------*/
