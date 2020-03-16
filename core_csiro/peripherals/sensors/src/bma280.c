/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "bma280.h"
#include "bma280_device.h"

#include "csiro_math.h"
#include "gpio.h"
#include "log.h"
#include "memory_operations.h"
#include "rtc.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define BMA280_READ			0x80
#define BMA280_WRITE		0x00

// clang-format on
/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

static void prvBma280DataReadyIRQ( void );
static void prvBma280EventIRQ( void );

static eModuleError_t eWriteRegister( uint8_t ucRegister, uint8_t ucData, TickType_t xTimeout );
static eModuleError_t eReadRegisters( uint8_t ucRegister, uint8_t *pucData, uint8_t ucLength, TickType_t xTimeout );

static eModuleError_t prvBmaConfigureData( xAccelerometerConfiguration_t *pxConfig, xAccelerometerState_t *pxState );
static void			  prvBmaConfigureEvents( xAccelerometerConfiguration_t *pxConfig, xAccelerometerState_t *pxState );

/* Private Variables ----------------------------------------*/

static xSpiModule_t *pxModule;

static xSpiConfig_t xBmaBusConfig = {
	.ulMaxBitrate = 10000000, // Max for BMA280 is 10MHz
	.ucDummyTx	= 0xFF,
	.ucMsbFirst   = true,
	.xCsGpio	  = { 0 },
	.eClockMode   = eSpiClockMode0
};

static QueueHandle_t xInterruptQueue;
static xGpio_t		 xInterrupt1;
static xGpio_t		 xInterrupt2;

static uint8_t ucCurrentRangeShift = 0;

static xTdfTime_t xInterruptTime;
static uint64_t   ullPreviousInterrupt;
static uint32_t   ulInterruptPeriod;

/*-----------------------------------------------------------*/

eModuleError_t eBma280Init( xBma280Init_t *pxInit, TickType_t xTimeout )
{
	eModuleError_t eError;

	configASSERT( xTimeout > pdMS_TO_TICKS( 5 ) );

	/* Store Configuration */
	pxModule			  = pxInit->pxSpi;
	xBmaBusConfig.xCsGpio = pxInit->xChipSelect;
	xInterrupt1			  = pxInit->xInterrupt1;
	xInterrupt2			  = pxInit->xInterrupt2;

	/* Create a queue for accelerometer interrupts */
	xInterruptQueue = xQueueCreate( 2, sizeof( eAccelerometerInterrupt_t ) );

	/* Setup the interrupt pins */
	vGpioSetup( xInterrupt1, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	vGpioSetup( xInterrupt2, GPIO_DISABLED, GPIO_DISABLED_NOPULL );

	/* Wait for sensor to power up */
	vTaskDelay( pdMS_TO_TICKS( 4 ) + 1 ); // Startup time is 3ms

	/* Put Chip into low power state */
	eError = eWriteRegister( PMU_LPW, POWER_MODE_DEEP_SUSPEND | SLEEP_DURATION_1S, xTimeout );
	if ( eError == ERROR_NONE ) {
		eLog( LOG_IMU_DRIVER, LOG_INFO, "BMA280 Initialisation Complete\r\n" );
	}
	else {
		eLog( LOG_IMU_DRIVER, LOG_ERROR, "BMA280 Initialisation Failed\r\n" );
	}
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eBma280WhoAmI( uint8_t *pucWhoAmI, TickType_t xTimeout )
{
	eModuleError_t eError;

	/* Read the chip ID */
	eError = eReadRegisters( BGW_CHIP_ID, pucWhoAmI, 1, xTimeout );

	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eBma280Configure( xAccelerometerConfiguration_t *pxConfig, xAccelerometerState_t *pxState, TickType_t xTimeout )
{
	eAccelerometerInterrupt_t eIntType;
	eModuleError_t			  eError;

	UNUSED( eError );

	/* Disable both interrupts and clear any pending interrupts */
	eGpioConfigureInterrupt( xInterrupt1, false, GPIO_INTERRUPT_BOTH_EDGE, NULL );
	eGpioConfigureInterrupt( xInterrupt2, false, GPIO_INTERRUPT_BOTH_EDGE, NULL );
	vGpioSetup( xInterrupt1, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	vGpioSetup( xInterrupt2, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	xQueueReceive( xInterruptQueue, &eIntType, 0 );

	/* Claim SPI bus for entire configuration time */
	if ( eSpiBusLockout( pxModule, true, xTimeout ) == ERROR_TIMEOUT ) {
		return ERROR_TIMEOUT;
	}

	/* We can return to BMA280 "NORMAL Mode" from any state with the SOFTRESET command */
	eWriteRegister( BGW_SOFTRESET, SOFT_RESET_VALUE, portMAX_DELAY );

	/* Wait for the chip to reset, maximum of 1.8ms */
	vTaskDelay( pdMS_TO_TICKS( 2 ) + 1 );

	/* Check for low power */
	if ( pxConfig->bEnabled == false ) {
		/* Move to "DEEP-SUSPEND Mode" */
		eWriteRegister( PMU_LPW, POWER_MODE_DEEP_SUSPEND | SLEEP_DURATION_1S, portMAX_DELAY );
		eSpiBusLockout( pxModule, false, portMAX_DELAY );
		pvMemset( pxState, 0x00, sizeof( xAccelerometerState_t ) );
		return ERROR_NONE;
	}

	eError = prvBmaConfigureData( pxConfig, pxState );
	if ( eError != ERROR_NONE ) {
		eSpiBusLockout( pxModule, false, portMAX_DELAY );
		return eError;
	}
	prvBmaConfigureEvents( pxConfig, pxState );

	/* Release SPI bus */
	eSpiBusLockout( pxModule, false, portMAX_DELAY );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBma280ReadData( xAccelerometerSample_t *pxData, xTdfTime_t *pxFirstSample, uint32_t *pulGenerationTime, uint8_t ucNumFIFO, TickType_t xTimeout )
{
	eModuleError_t eError;
	struct xHardwareSample_t
	{
		int16_t sX;
		int16_t sY;
		int16_t sZ;
	};
	struct xHardwareSample_t *pxHwSamples = (struct xHardwareSample_t *) pxData;

	if ( ucNumFIFO == 0 ) {
		/* Burst read data registers */
		eError = eReadRegisters( ACCD_X_LSB, (uint8_t *) pxData, 6, xTimeout );
	}
	else {
		/* Burst read FIFO registers */
		eError = eReadRegisters( FIFO_DATA, (uint8_t *) pxData, 6 * ucNumFIFO, xTimeout );
	}

	/* Repack the 16bit samples into 32 bit samples */
	uint8_t					  ucSampleIndex = ( ucNumFIFO == 0 ) ? 0 : ucNumFIFO - 1;
	xAccelerometerSample_t *  pxOutputSample;
	struct xHardwareSample_t *pxHwSample;
	do {
		/* Loop over every sample in reverse order */
		pxOutputSample = pxData + ucSampleIndex;
		pxHwSample	 = pxHwSamples + ucSampleIndex;

		/** 
		 * Calculate the magnitude before we destroy the hardware sample in memory
		 * 
		 * We know there is only 14 bits of useful information here, 
		 * so we can perform the magnitude calculation on the 14 bits of data, then shift it afterwards
		 * 
		 * We can gaurantee this wont overflow because:
		 * 		3 * ((2 ** 14) ** 2) == 3 * (2 ** 28) < 2 ** 32
		 **/
		uint32_t ulSquaredMag = VECTOR_SQR_MAGNITUDE( pxHwSample->sX >> 2, pxHwSample->sY >> 2, pxHwSample->sZ >> 2 );
		uint32_t ulMagnitude  = ulSquareRoot( ulSquaredMag );
		/** 
		 * On the last sample we need to worry about the pointers overlapping 
		 * The output sample will be 16 bytes, the first 6 bytes of which contains the compressed data
		 * Unpack in order Z->Y->X
		 * 
		 * 14 bit samples sitting in the top 14 bits of a 16 bit number
		 * The lowest 2 bits are essentially undefined, and must be cleared
		 * After clearing, the sample looks like a 16 bit sample
		 * 
		 * Apply the required shift for the current maximum G range
		 **/
		pxOutputSample->lZ			= ( ( (int32_t) pxHwSample->sZ ) & ~0x3 ) << ucCurrentRangeShift;
		pxOutputSample->lY			= ( ( (int32_t) pxHwSample->sY ) & ~0x3 ) << ucCurrentRangeShift;
		pxOutputSample->lX			= ( ( (int32_t) pxHwSample->sX ) & ~0x3 ) << ucCurrentRangeShift;
		pxOutputSample->ulMagnitude = ulMagnitude << ( 2 + ucCurrentRangeShift );
	} while ( ucSampleIndex-- );

	/* Timing information */
	*pulGenerationTime = ulInterruptPeriod;
	if ( ucNumFIFO > 1 ) {
		uint32_t   ulFirstSampleTimeAgoTdfTicks = ( 2 * ulInterruptPeriod ) - ( 2 * ulInterruptPeriod / ucNumFIFO );
		xTdfTime_t xFirstDelta					= { ulFirstSampleTimeAgoTdfTicks / UINT16_MAX, ulFirstSampleTimeAgoTdfTicks % UINT16_MAX };
		*pxFirstSample							= xRtcTdfTimeSub( xInterruptTime, xFirstDelta );
	}
	else {
		*pxFirstSample = xInterruptTime;
	}

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Basic 'Wait for Interrupt' function.
 *
 * Waits for an interrupt from the BMA280 chip.
 */
eModuleError_t eBma280WaitForInterrupt( eAccelerometerInterrupt_t *peInterruptType, TickType_t xTimeout )
{
	uint8_t ucIntStatus;

	if ( xQueueReceive( xInterruptQueue, peInterruptType, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}
	/* Something other than the typical data ready interrupt occured */
	if ( *peInterruptType == ACCELEROMETER_OTHER ) {
		/* Currently the only other interrupt we setup is SLO_NO_MOT, flag is in INT_STATUS_0 */
		eReadRegisters( INT_STATUS_0, &ucIntStatus, 1, portMAX_DELAY );
		if ( ucIntStatus & SLO_NOT_MOT_INT ) {
			*peInterruptType = ACCELEROMETER_NO_MOTION;
		}
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBma280ActiveInterrupts( eAccelerometerInterrupt_t *peInterrupts, TickType_t xTimeout )
{
	uint8_t		   ucIntStatus = 0x00;
	eModuleError_t eError	  = eReadRegisters( INT_STATUS_0, &ucIntStatus, 1, xTimeout );
	/* Only the SLO_NO_MOT interrupt is currently utilised */
	*peInterrupts = 0x00;
	if ( ucIntStatus & SLO_NOT_MOT_INT ) {
		*peInterrupts = ACCELEROMETER_NO_MOTION;
	}
	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Basic Interrupt Service Routine.
 */
void prvBma280DataReadyIRQ( void )
{
	BaseType_t				  xHigherPriorityTaskWoken = pdFALSE;
	eAccelerometerInterrupt_t eIntType				   = ACCELEROMETER_NEW_DATA;
	uint64_t				  ullRtcNow				   = ullRtcTickCount();
	ulInterruptPeriod								   = ullRtcNow - ullPreviousInterrupt;
	ullPreviousInterrupt							   = ullRtcNow;
	bRtcGetTdfTime( &xInterruptTime );
	xQueueSendToBackFromISR( xInterruptQueue, &eIntType, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

/**
 * Event Interrupt Service Routine.
 */
void prvBma280EventIRQ( void )
{
	BaseType_t				  xHigherPriorityTaskWoken = pdFALSE;
	eAccelerometerInterrupt_t eIntType				   = ACCELEROMETER_OTHER;
	xQueueSendToBackFromISR( xInterruptQueue, &eIntType, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

static eModuleError_t prvBmaConfigureData( xAccelerometerConfiguration_t *pxConfig, xAccelerometerState_t *pxState )
{
	/* Apply the desired maximum acceleration range */
	const uint8_t pucRanges[4]		= { 2, 4, 8, 16 };
	const uint8_t pucOutputRange[4] = { REGISTER_RANGE_2G, REGISTER_RANGE_4G, REGISTER_RANGE_8G, REGISTER_RANGE_16G };
	uint32_t	  ulRange			= ulArraySearchByte( pucRanges, pxConfig->ucRangeG, 4 );
	ucCurrentRangeShift				= ulRange; /* Bitshift corresponds to the index in pucRanges */
	if ( ulRange == UINT32_MAX ) {
		/* Provided range wasn't valid */
		pvMemset( pxState, 0x00, sizeof( xAccelerometerState_t ) );
		return ERROR_INVALID_DATA;
	}
	eWriteRegister( PMU_RANGE, pucOutputRange[ulRange], portMAX_DELAY );

	/* Sample Rate and Power Modes */
	uint32_t ulSampleRate;
	if ( pxConfig->bLowPowerMode ) {
		/* Round sample rates down to a supported rate and convert to register value */
		const uint8_t pucLowPowerRates[7] = { 1, 2, 10, 20, 40, 100, 166 };
		const uint8_t pucRegisterRates[7] = { SLEEP_DURATION_1S, SLEEP_DURATION_500MS, SLEEP_DURATION_100MS, SLEEP_DURATION_50MS, SLEEP_DURATION_25MS, SLEEP_DURATION_10MS, SLEEP_DURATION_6MS };
		uint8_t		  ucRateIndex		  = ucBinIndexByte( (uint8_t) pxConfig->usSampleRateHz, pucLowPowerRates, 7 );
		ucRateIndex						  = ucRateIndex > 0 ? ucRateIndex - 1 : ucRateIndex;
		uint8_t ucRateRegister			  = POWER_MODE_LOW_POWER | pucRegisterRates[ucRateIndex];
		ulSampleRate					  = 1000 * (uint32_t) pucLowPowerRates[ucRateIndex];

		/* Higher bandwidths (Faster ODR) results in lower current, however unfiltered data results in weird 2-peak gaussian distribution in output samples */
		eWriteRegister( PMU_BW, FILTER_BANDWIDTH_500H, portMAX_DELAY );   /** Bandwidth ( Sample Rate ) */
		eWriteRegister( PMU_LOW_POWER, LOW_POWER_MODE_1, portMAX_DELAY ); /** Low Power Mode 1 */
		eWriteRegister( PMU_LPW, ucRateRegister, portMAX_DELAY );		  /** Sleep Durations */

		/* Disable Data shadowing */
		eWriteRegister( ACCD_HBW, BMA280_SHADOWING_DISABLE, portMAX_DELAY );

		/* Typical data interrupts */
		eWriteRegister( INT_MAP_1, BMA280_INT_MAP_1_INT1_DATA, portMAX_DELAY ); /** DATA_READY Interrupt */
		eWriteRegister( INT_EN_1, BMA280_INT_EN_1_EN_DATA, portMAX_DELAY );		/** DATA_READY Interrupt Enable */
	}
	else {
		/* Round sample rates down to a supported rate and convert to register value */
		const uint32_t pulHighPowerRates[8] = { 15630, 31250, 62500, 125000, 250000, 500000, 1000000, 2000000 };
		const uint8_t  pucRegisterRates[8]  = { FILTER_BANDWIDTH_7H81, FILTER_BANDWIDTH_15H63, FILTER_BANDWIDTH_31H25, FILTER_BANDWIDTH_62H5, FILTER_BANDWIDTH_125H, FILTER_BANDWIDTH_250H, FILTER_BANDWIDTH_500H, FILTER_BANDWIDTH_UNFILTERED };
		uint8_t		   ucRateIndex			= ucBinIndexLong( 1000 * (uint32_t) pxConfig->usSampleRateHz, pulHighPowerRates, 4 );
		ucRateIndex							= ucRateIndex > 0 ? ucRateIndex - 1 : ucRateIndex;
		uint8_t ucRateRegister				= pucRegisterRates[ucRateIndex];
		ulSampleRate						= pulHighPowerRates[ucRateIndex];

		/* Data configuration for high power modes */
		eWriteRegister( PMU_BW, ucRateRegister, portMAX_DELAY );	 /** Bandwidth ( Sample Rate ) */
		eWriteRegister( PMU_LPW, POWER_MODE_NORMAL, portMAX_DELAY ); /** Sleep Duration (None) */

		if ( pxConfig->ucFIFOLimit == 0 ) {
			/* Interrupt on every sample */
			eWriteRegister( INT_MAP_1, BMA280_INT_MAP_1_INT1_DATA, portMAX_DELAY ); /** DATA_READY Interrupt */
			eWriteRegister( INT_EN_1, BMA280_INT_EN_1_EN_DATA, portMAX_DELAY );		/** DATA_READY Interrupt Enable */
		}
		else if ( pxConfig->ucFIFOLimit < 32 ) {
			/* Interrupt when FIFO reaches supplied level */
			eWriteRegister( FIFO_CONFIG_0, pxConfig->ucFIFOLimit, portMAX_DELAY ); /* FIFO watermark interrupt level */
			eWriteRegister( FIFO_CONFIG_1, 0x40, portMAX_DELAY );				   /* 3 axis FIFO Mode*/
			/* Interrupt on FIFO watermark */
			eWriteRegister( INT_MAP_1, BMA280_INT_MAP_1_INT1_FWM, portMAX_DELAY ); /** FIFO Watermark Interrupt */
			eWriteRegister( INT_EN_1, BMA280_INT_EN_1_EN_FWM, portMAX_DELAY );	 /** FIFO Watermark Interrupt Enable */
		}
		else {
			/* Interrupt when FIFO is full */
			eWriteRegister( FIFO_CONFIG_1, 0x40, portMAX_DELAY ); /* 3 axis FIFO Mode*/
			/* Interrupt on FIFO Full */
			eWriteRegister( INT_MAP_1, BMA280_INT_MAP_1_INT1_FFULL, portMAX_DELAY ); /** FIFO Full Interrupt */
			eWriteRegister( INT_EN_1, BMA280_INT_EN_1_EN_FFULL, portMAX_DELAY );	 /** FIFO Full Interrupt Enable */
		}
	}
	/* Enable interrupt pins */
	vGpioSetup( xInterrupt1, GPIO_INPUT, GPIO_INPUT_NOFILTER );
	eGpioConfigureInterrupt( xInterrupt1, true, GPIO_INTERRUPT_RISING_EDGE, prvBma280DataReadyIRQ );
	/* Set an approximate previous interrupt time */
	ullPreviousInterrupt = ullRtcTickCount();
	/* Store actual configuration */
	pxState->bEnabled		  = true;
	pxState->ucSampleGrouping = CLAMP( pxConfig->ucFIFOLimit, 32, 1 );
	pxState->ucMaxG			  = pucRanges[ulRange];
	pxState->ulRateMilliHz	= ulSampleRate;
	pxState->ulPeriodUs		  = 1000000000 / pxState->ulRateMilliHz;
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static void prvBmaConfigureEvents( xAccelerometerConfiguration_t *pxConfig, xAccelerometerState_t *pxState )
{
	UNUSED( pxState );

	/* We want interrupts to be transient events */
	eWriteRegister( INT_RST_LATCH, BMA280_LATCH_250US, portMAX_DELAY );

	/* No activity detection configuration */
	if ( pxConfig->xNoActivityConfig.bEnabled ) {
		/* Pin needs to be assigned to work */
		configASSERT( !bGpioEqual( xInterrupt2, UNUSED_GPIO ) );
		/* Interrupts in low power mode result in undesirable behaviour */
		configASSERT( !pxConfig->bLowPowerMode );
		/* Convert seconds to annoying stepwise function from datasheet */
		uint8_t ucDurationValue;
		if ( pxConfig->xNoActivityConfig.usDurationS < 40 ) {
			/* Second resolution, maximum of 16 seconds (16-40 seconds not representable) */
			ucDurationValue = MIN( 15, pxConfig->xNoActivityConfig.usDurationS - 1 );
		}
		else if ( pxConfig->xNoActivityConfig.usDurationS < 88 ) {
			/* 8 second resolution */
			ucDurationValue = ( pxConfig->xNoActivityConfig.usDurationS / 8 ) + 11;
		}
		else {
			/* Also 8 second resolution, jump in representable range in the register */
			ucDurationValue = ( MIN( pxConfig->xNoActivityConfig.usDurationS, 336 ) / 8 ) + 21;
		}
		/* Threshold to LSB, calculate 2G threshold first then scale it */
		uint8_t ucNoMotionThreshold = 100 * pxConfig->xNoActivityConfig.usThresholdMilliG / 391;
		ucNoMotionThreshold			= ucNoMotionThreshold >> ucCurrentRangeShift;
		ucNoMotionThreshold			= MAX( 1, ucNoMotionThreshold );
		/* Write configuration to registers */
		eWriteRegister( INT_5, ucDurationValue << 2, portMAX_DELAY );
		eWriteRegister( INT_7, ucNoMotionThreshold, portMAX_DELAY );
		/* Map No-Motion interrupt to INT2 */
		eWriteRegister( INT_MAP_2, BMA280_INT_MAP_SLOW_NO_MOTION, portMAX_DELAY );
		/**< No Motion mode selection */
		uint8_t ucNoMotionIntConfig = BMA280_INT_EN_2_SLOW_NO_MOTION_SEL | BMA280_INT_EN_2_EN_SLOW_NO_MOTION_X | BMA280_INT_EN_2_EN_SLOW_NO_MOTION_Y | BMA280_INT_EN_2_EN_SLOW_NO_MOTION_Z;
		eWriteRegister( INT_EN_2, ucNoMotionIntConfig, portMAX_DELAY );
		/* Setup interrupt pins */
		vGpioSetup( xInterrupt2, GPIO_INPUT, GPIO_INPUT_NOFILTER );
		eGpioConfigureInterrupt( xInterrupt2, true, GPIO_INTERRUPT_RISING_EDGE, prvBma280EventIRQ );
	}
}

/*-----------------------------------------------------------*/

static eModuleError_t eReadRegisters( uint8_t ucRegister, uint8_t *pucData, uint8_t ucLength, TickType_t xTimeout )
{
	eModuleError_t eError;
	uint8_t		   ucCommand[1] = { BMA280_READ | ucRegister };

	eError = eSpiBusStart( pxModule, &xBmaBusConfig, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}
	vSpiCsAssert( pxModule );

	/* Transmit and Receive Data */
	vSpiTransmit( pxModule, ucCommand, 1 );
	vSpiReceive( pxModule, pucData, ucLength );

	vSpiCsRelease( pxModule );
	vSpiBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t eWriteRegister( uint8_t ucRegister, uint8_t ucData, TickType_t xTimeout )
{
	uint8_t		   pucCommand[2] = { ( BMA280_WRITE | ucRegister ), ucData };
	eModuleError_t eError;

	eError = eSpiBusStart( pxModule, &xBmaBusConfig, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}
	vSpiCsAssert( pxModule );

	/* Transfer Data */
	vSpiTransmit( pxModule, pucCommand, 2 );

	vSpiCsRelease( pxModule );
	vSpiBusEnd( pxModule );

	/**
	 * LPM1 requires 450uS between writes
	 * Two ticks gaurantee this duration passes for all tick rates below 2200 Hz
	 **/
	vTaskDelay( 2 );
	return eError;
}

/*-----------------------------------------------------------*/
