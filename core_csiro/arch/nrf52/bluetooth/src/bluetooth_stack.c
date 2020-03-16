/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "bluetooth_types.h"

#include "bluetooth_controller.h"
#include "log.h"

#include "address.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_gatts.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"

/* Private Defines ------------------------------------------*/

#define EVENT_BUFFER_SIZE BLE_EVT_LEN_MAX( BLUETOOTH_GATT_MAX_MTU )

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void prvBluetoothUUIDTableFind( uint8_t *pucRamStart, uint8_t *pucRamEnd );

static void prvBtStackTask( void *pvParameters );

void prvBluetoothGattEventHandler( ble_evt_t const *pxEvent );
void prvBluetoothGapEventHandler( ble_evt_t const *pxEvent );

void nrf_fstorage_sys_evt_handler( uint32_t sys_evt, void *p_context );
void nrf_fstorage_sdh_state_handler( nrf_sdh_state_evt_t state, void *p_context );

void assertion_failed( uint32_t id, uint32_t pc, uint32_t info );

/* Private Variables ----------------------------------------*/

/* Random UUID's used for table search */
static const uint8_t pucUniqueUUID[16]  = { 0x40, 0xBF, 0x0B, 0xAB, 0xB8, 0x0B, 0x4B, 0xE6, 0x83, 0xB6, 0xCD, 0x7D, 0x39, 0x5B, 0x04, 0x90 };
static const uint8_t pucUniqueUUID2[16] = { 0x40, 0xBF, 0x0B, 0xAB, 0xB8, 0x0B, 0x4B, 0xE6, 0x83, 0xB6, 0xCD, 0x7D, 0x39, 0x5B, 0x04, 0x23 };

/* Softdevice UUID Table parameters */
static uint8_t *pucVsUUIDTable		= NULL;
static uint32_t ulVsUUIDTableOffset = 0;

const nrf_clock_lf_cfg_t clock_lf_cfg = {
	.source		  = NRF_CLOCK_LF_SRC_XTAL,
	.rc_ctiv	  = 0,
	.rc_temp_ctiv = 0,
	.accuracy	 = NRF_CLOCK_LF_ACCURACY_20_PPM
};

STATIC_TASK_STRUCTURES( pxBtStackHandle, 2 * configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 5 );

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothInit( void )
{
	ble_cfg_t			  ble_cfg;
	extern uint32_t		  __softdevice_ram_end__;
	uint32_t const *const pulApplicationRamStart = &__softdevice_ram_end__;
	uint32_t			  ret_code;
	/* SD calls take a pointer to the address, not the address itself */
	uint32_t ulApplicationRamStart = (uint32_t) pulApplicationRamStart;

	/* Start the task that handles softdevice events */
	STATIC_TASK_CREATE( pxBtStackHandle, prvBtStackTask, "BT STACK", NULL );

	/* Initialise the bluetooth controller */
	vBluetoothControllerInit();

	/* Enable the Softdevice */
	ret_code = sd_softdevice_enable( &clock_lf_cfg, assertion_failed );
	configASSERT( ret_code == NRF_SUCCESS );

	/* Enable and configure the Softdevice interrupt line */
	vInterruptSetPriority( SD_EVT_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptEnable( SD_EVT_IRQn );

	/* Configure GAP parameters */
	pvMemset( &ble_cfg, 0x00, sizeof( ble_cfg ) );
	ble_cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 8;
	ret_code									 = sd_ble_cfg_set( BLE_COMMON_CFG_VS_UUID, &ble_cfg, ulApplicationRamStart );
	configASSERT( ret_code == NRF_SUCCESS );

	pvMemset( &ble_cfg, 0x00, sizeof( ble_cfg ) );
	ble_cfg.gatts_cfg.attr_tab_size.attr_tab_size = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
	ret_code									  = sd_ble_cfg_set( BLE_GATTS_CFG_ATTR_TAB_SIZE, &ble_cfg, ulApplicationRamStart );
	configASSERT( ret_code == NRF_SUCCESS );

	/* Configure Connection Parameters */
	pvMemset( &ble_cfg, 0x00, sizeof( ble_cfg ) );
	ble_cfg.gatts_cfg.service_changed.service_changed = 0;
	ret_code										  = sd_ble_cfg_set( BLE_GATTS_CFG_SERVICE_CHANGED, &ble_cfg, ulApplicationRamStart );
	configASSERT( ret_code == NRF_SUCCESS );

	pvMemset( &ble_cfg, 0x00, sizeof( ble_cfg ) );
	ble_cfg.conn_cfg.conn_cfg_tag					  = CSIRO_CONNECTION_TAG;
	ble_cfg.conn_cfg.params.gap_conn_cfg.event_length = 6;
	ble_cfg.conn_cfg.params.gap_conn_cfg.conn_count   = BLE_GAP_CONN_COUNT_DEFAULT;
	ret_code										  = sd_ble_cfg_set( BLE_CONN_CFG_GAP, &ble_cfg, ulApplicationRamStart );
	configASSERT( ret_code == NRF_SUCCESS );

	pvMemset( &ble_cfg, 0x00, sizeof( ble_cfg ) );
	ble_cfg.conn_cfg.conn_cfg_tag				  = CSIRO_CONNECTION_TAG;
	ble_cfg.conn_cfg.params.gatt_conn_cfg.att_mtu = BLUETOOTH_GATT_MAX_MTU;
	ret_code									  = sd_ble_cfg_set( BLE_CONN_CFG_GATT, &ble_cfg, ulApplicationRamStart );
	configASSERT( ret_code == NRF_SUCCESS );

	/* Enable Bluetooth stack */
	ret_code = sd_ble_enable( &ulApplicationRamStart );

	// /* Configure GAP parameters requiring sd_ble_enable */
	// pvMemset( &ble_cfg, 0x00, sizeof( ble_cfg ) );
	// ble_cfg.gap_cfg.role_count_cfg.adv_set_count					 = 1;
	// ble_cfg.gap_cfg.role_count_cfg.central_role_count				 = 3;
	// ble_cfg.gap_cfg.role_count_cfg.central_sec_count				 = 1;
	// ble_cfg.gap_cfg.role_count_cfg.periph_role_count				 = 3;
	// ble_cfg.gap_cfg.role_count_cfg.qos_channel_survey_role_available = 0;
	// ret_code														 = sd_ble_cfg_set( BLE_GAP_CFG_ROLE_COUNT, &ble_cfg, ulApplicationRamStart );

	prvBluetoothUUIDTableFind( (uint8_t *) 0x20000000, (uint8_t *) pulApplicationRamStart );

	configASSERT( ret_code == NRF_SUCCESS );
	return ret_code == NRF_SUCCESS ? ERROR_NONE : ERROR_INITIALISATION_FAILURE;
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void prvBtStackTask( void *pvParameters )
{
	UNUSED( pvParameters );
	uint8_t pucEventBuffer[EVENT_BUFFER_SIZE] ATTR_ALIGNED( __alignof__( ble_evt_t ) );

	uint16_t   usEventLen;
	uint32_t   ulRet;
	ble_evt_t *pxBluetoothEvt;
	uint32_t   ulSocEvent;

	nrf_fstorage_sdh_state_handler( NRF_SDH_EVT_STATE_ENABLED, NULL );

	for ( ;; ) {
		ulTaskNotifyTake( pdFALSE, portMAX_DELAY );
		/* Must loop until sd_ble_evt_get returns NRF_ERROR_NOT_FOUND */
		while ( true ) {
			usEventLen = EVENT_BUFFER_SIZE;
			ulRet	  = sd_ble_evt_get( pucEventBuffer, &usEventLen );
			if ( ulRet != NRF_SUCCESS ) {
				break;
			}
			/* 
			 * We can gaurantee correct alignment of this cast as we explicitly 
			 * specify the required alignment when the variable is declared 
			 */
			COMPILER_WARNING_DISABLE( "-Wcast-align" );
			pxBluetoothEvt = (ble_evt_t *) pucEventBuffer;
			COMPILER_WARNING_ENABLE();

			/* GAP Handler */
			prvBluetoothGapEventHandler( pxBluetoothEvt );
			/* GATT Handler */
			prvBluetoothGattEventHandler( pxBluetoothEvt );
		}
		/* NRF_ERROR_NOT_FOUND is the expected terminating condition */
		configASSERT( ulRet == NRF_ERROR_NOT_FOUND );
		/* Loop while SoC has events */
		while ( true ) {
			ulRet = sd_evt_get( &ulSocEvent );
			if ( ulRet != NRF_SUCCESS ) {
				break;
			}
			nrf_fstorage_sys_evt_handler( ulSocEvent, NULL );
		}
		/* NRF_ERROR_NOT_FOUND is the expected terminating condition */
		configASSERT( ulRet == NRF_ERROR_NOT_FOUND );
	}
}

/*-----------------------------------------------------------*/

void SD_EVT_IRQHandler( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR( pxBtStackHandle, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothStackOn( void )
{
	/* Nothing needs to be done to bring bluetooth out of low power mode */
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothStackOff( void )
{
	/* Nothing needs to be done to put bluetooth to low power mode */
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/* Softdevice is always enabled */
bool nrf_sdh_is_enabled( void )
{
	return true;
}

/*-----------------------------------------------------------*/

/* soft device never needs to be resarted */
ret_code_t nrf_sdh_request_continue( void )
{
	return NRF_SUCCESS;
}

/*-----------------------------------------------------------*/

void assertion_failed( uint32_t id, uint32_t pc, uint32_t info )
{
	UNUSED( id );
	UNUSED( info );
	vAssertionFailed( __FILE__, __LINE__, pc, 0 );
}

/*-----------------------------------------------------------*/

void app_error_handler_bare( ret_code_t error_code )
{
	UNUSED( error_code );
	configASSERT( 0 );
}

/*-----------------------------------------------------------*/

void app_error_fault_handler( uint32_t id, uint32_t pc, uint32_t info )
{
	UNUSED( id );
	UNUSED( pc );
	UNUSED( info );
	configASSERT( 0 );
}

/*-----------------------------------------------------------*/

void vBluetoothStackUUIDResolve( struct xBluetoothUUID_t *pxUUID )
{
	/* Only need to resolve custom UUID's */
	if ( !pxUUID->bBluetoothOfficialUUID ) {
		/* Reference 0 is Unknown, 1 is Bluetooth Official, 2 is the start of Vendor Specific bases */
		const uint8_t  ucTableOffset = pxUUID->uUUID.xCustomUUID.ucStackReference - 2;
		const uint8_t *pucUUID		 = pucVsUUIDTable + ( ucTableOffset * ulVsUUIDTableOffset );
		pvMemcpy( pxUUID->uUUID.xCustomUUID.pucUUID128, pucUUID, 16 );
	}
}

/*-----------------------------------------------------------*/

/**
 * To avoid having to store excessive state between connections we need to be able to convert the 8bit 'Vendor Specific' UUID code
 * back into the actual 128bit UUID. The softdevice doesn't provide any mechanism for doing this, so we exploit the fact that the
 * 'Vendor Specific' lists are dynamic, and therefore must be stored in RAM somewhere.
 * 
 * This function pushes two unique, random 128bit UUID's into the stack and then searches through RAM to find where they were put.
 * Additionally, the offset between the two UUID's is checked to handle any padding which may be inserted between different UUID's
 * 
 * To operate correctly this function needs to be called before any other Vendor Specific UUID's are added to the stack.
 * 
 * Softdevice updates will either pass gracefully or hit one of the asserts in the case that the internal data format changes.
 * The only possible silent error is if the UUIDs are not stored at consitent offsets, i.e.
 * 			VS1 = BASE
 * 			VS2 = BASE + 16
 * 			VS3 = BASE + 48
 * 
 * There is no reason why it would be changed to be this way though
 */
void prvBluetoothUUIDTableFind( uint8_t *pucRamStart, uint8_t *pucRamEnd )
{
	ble_uuid128_t xUUID;
	uint8_t		  ucTableIndex;

	/* Add two Vendor Specific 128 bit UUID's to the stack */
	pvMemcpy( xUUID.uuid128, pucUniqueUUID, 16 );
	configASSERT( sd_ble_uuid_vs_add( &xUUID, &ucTableIndex ) == NRF_SUCCESS );
	pvMemcpy( xUUID.uuid128, pucUniqueUUID2, 16 );
	configASSERT( sd_ble_uuid_vs_add( &xUUID, &ucTableIndex ) == NRF_SUCCESS );

	/* Search through the RAM space provided for the two UUIDs */
	while ( pucRamStart < pucRamEnd ) {
		if ( lMemcmp( pucUniqueUUID, (uint8_t *) pucRamStart, 16 ) == 0 ) {
			pucVsUUIDTable = pucRamStart;
		}
		if ( lMemcmp( pucUniqueUUID2, (uint8_t *) pucRamStart, 16 ) == 0 ) {
			ulVsUUIDTableOffset = pucRamStart - pucVsUUIDTable;
			break;
		}
		/* Assume that the table is at least word aligned */
		pucRamStart += 4;
	}
	/* Validate that the search succeeded */
	configASSERT( pucVsUUIDTable != NULL );
	configASSERT( ulVsUUIDTableOffset != 0 );

	/* Remove the useless VS bases from the stack */
	ucTableIndex = BLE_UUID_TYPE_UNKNOWN;
	configASSERT( sd_ble_uuid_vs_remove( &ucTableIndex ) == NRF_SUCCESS );
	ucTableIndex = BLE_UUID_TYPE_UNKNOWN;
	configASSERT( sd_ble_uuid_vs_remove( &ucTableIndex ) == NRF_SUCCESS );
}

/*-----------------------------------------------------------*/
