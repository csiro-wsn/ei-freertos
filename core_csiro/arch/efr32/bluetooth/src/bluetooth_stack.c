/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 * 
 * Bluetooth Tasks implemented to spec described in
 * https://www.silabs.com/documents/login/application-notes/an1114-integrating-bluetooth-applications-with-rtos.pdf
 * 
 * 
 * !!!!!!!!!!!!!!!!!!!!!!
 * !!!!!!!! TODO !!!!!!!!
 * !!!!!!!!!!!!!!!!!!!!!!
 * Believe the reason the stack is mallocing on a connection is due to the remote size requesting a larger PDU.
 * On nRF52 side this is triggered by data_length_extension (DLE) request
 * Need to figure out a way to pre trigger this on initialisation.
 * 
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "task.h"

#include "bluetooth_types.h"

#include "bluetooth_controller.h"

#include "em_chip.h"
#include "gatt_efr32.h"
#include "rtos_gecko.h"

#include "compiler_intrinsics.h"
#include "cpu.h"
#include "log.h"

/* Private Defines ------------------------------------------*/

// clang-format off
#define BLUETOOTH_TICK_HZ  32768
#define BLUETOOTH_TO_RTOS_TICK  			(BLUETOOTH_TICK_HZ / configTICK_RATE_HZ)

#define BLUETOOTH_EVENT_FLAG_STACK          (0x01)   //Bluetooth task needs an update
#define BLUETOOTH_EVENT_FLAG_LL             (0x02)   //Linklayer task needs an update
#define BLUETOOTH_EVENT_FLAG_CMD_WAITING    (0x04)   //BGAPI command is waiting to be processed
#define BLUETOOTH_EVENT_FLAG_RSP_WAITING    (0x08)   //BGAPI response is waiting to be processed
#define BLUETOOTH_EVENT_FLAG_EVT_WAITING    (0x10)   //BGAPI event is waiting to be processed
#define BLUETOOTH_EVENT_FLAG_EVT_HANDLED    (0x20)   //BGAPI event is handled

#define BT_MAX_CONNECTIONS					2

// clang-format on

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

static void prvBtAppTask( void *pvParameters );
static void prvBtHostTask( void *pvParameters );
static void prvBtLinkLayerTask( void *pvParameters );

static void vBluetoothUpdate( void );
static void vBluetoothLLCallback( void );

void vBluetoothGapEventHandler( struct gecko_cmd_packet *pxEvent );
void vBluetoothGattEventHandler( struct gecko_cmd_packet *pxEvent );

inline static void sli_bt_cmd_handler_delegate_volatile( uint32_t header, void ( *cmd_handler )( const void * ), volatile void *command_data );

/* Private Variables ----------------------------------------*/

// RTOS Variables
static SemaphoreHandle_t xBtMutexHandle = NULL;
static StaticSemaphore_t xBtMutex;

STATIC_TASK_STRUCTURES( pxBtAppHandle, configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 5 );
STATIC_TASK_STRUCTURES( pxBtHostHandle, configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 4 ); /* Must be lower priority that LinkLayer Task */
STATIC_TASK_STRUCTURES( pxBtLinkLayerHandle, configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 6 );

static EventGroupHandle_t xBtEventHandle;
static StaticEventGroup_t xBtEventGroup;

static TaskHandle_t xInitHandle;

static bool bStackOn = false;

// BT Variables
static volatile struct gecko_cmd_packet *bluetooth_evt;

static volatile uint32_t		  command_header;
static volatile void *			  command_data;
static volatile gecko_cmd_handler command_handler_func = NULL;

static int8_t cMaxTxPower;

static uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP( BT_MAX_CONNECTIONS )];

static const gecko_configuration_t bt_config = {
	.config_flags					= GECKO_CONFIG_FLAG_RTOS | GECKO_CONFIG_FLAG_NO_SLEEPDRV_INIT,
	.scheduler_callback				= vBluetoothLLCallback,
	.stack_schedule_callback		= vBluetoothUpdate,
	.sleep.flags					= SLEEP_FLAGS_DEEP_SLEEP_ENABLE,
	.bluetooth.max_connections		= BT_MAX_CONNECTIONS,
	.bluetooth.max_advertisers		= 1,
	.bluetooth.heap					= bluetooth_stack_heap,
	.bluetooth.heap_size			= sizeof( bluetooth_stack_heap ),
	.bluetooth.sleep_clock_accuracy = 100, // ppm
	.gattdb							= &bg_gattdb_data,
	.ota.flags						= 0,
	.ota.device_name_len			= 3,
	.ota.device_name_ptr			= (char *) "OTA",
	.max_timers						= 0
};

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothInit( void )
{
	struct gecko_msg_gatt_set_max_mtu_rsp_t *   pxMtuResp;
	struct gecko_msg_system_set_tx_power_rsp_t *pxPowerResp;

	eLog( LOG_BLUETOOTH_GAP, LOG_DEBUG, "Starting BT Tasks\r\n" );
	// Interrupt priorities MUST be limited for IRQ's that call FreeRTOS functions through callbacks
	vInterruptSetPriority( FRC_PRI_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptSetPriority( FRC_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptSetPriority( RAC_SEQ_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptSetPriority( BUFC_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptSetPriority( PROTIMER_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );

	xBtEventHandle = xEventGroupCreateStatic( &xBtEventGroup );
	// Create mutex, defult state is available
	xBtMutexHandle = xSemaphoreCreateMutexStatic( &xBtMutex );

	xInitHandle = xTaskGetCurrentTaskHandle();

	gecko_init( &bt_config );

	STATIC_TASK_CREATE( pxBtLinkLayerHandle, prvBtLinkLayerTask, "BT LL", NULL );
	STATIC_TASK_CREATE( pxBtAppHandle, prvBtAppTask, "BT APP", NULL );
	STATIC_TASK_CREATE( pxBtHostHandle, prvBtHostTask, "BT HST", NULL );

	/* Wait here for gecko stack to be initialised properly so we can call gecko_cmd functions */
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

	/* Update Max MTU */
	pxMtuResp = gecko_cmd_gatt_set_max_mtu( BLUETOOTH_GATT_MAX_MTU );
	configASSERT( pxMtuResp->result == 0 );

	int16_t sStackDbm = ( 10 * (int16_t) 20 );
	pxPowerResp		  = gecko_cmd_system_set_tx_power( sStackDbm );
	eLog( LOG_BLUETOOTH_GAP, LOG_INFO, "BT: Max TX power set to %d dBm\r\n", pxPowerResp->set_power / 10 );
	cMaxTxPower = pxPowerResp->set_power / 10;

	/* Set our Local Address */
	struct gecko_msg_system_get_bt_address_rsp_t *pxBtAddress;
	pxBtAddress   = gecko_cmd_system_get_bt_address();
	xLocalAddress = xAddressUnpack( pxBtAddress->address.addr );

	/* Initialise bluetooth controller */
	vBluetoothControllerInit();
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

bool bBluetoothCanDeepSleep( void )
{
	return ( bStackOn == false );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothStackOn( void )
{
	struct gecko_msg_system_halt_rsp_t *pxHaltResp;
	pxHaltResp = gecko_cmd_system_halt( 0 );
	bStackOn   = true;
	return ( pxHaltResp->result == 0 ) ? ERROR_NONE : ERROR_INVALID_STATE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothStackOff( void )
{
	struct gecko_msg_system_halt_rsp_t *pxHaltResp;
	pxHaltResp = gecko_cmd_system_halt( 1 );
	bStackOn   = false;
	return ( pxHaltResp->result == 1 ) ? ERROR_NONE : ERROR_INVALID_STATE;
}

/*-----------------------------------------------------------*/

int8_t cBluetoothStackGetValidTxPower( int8_t cRequestedPowerDbm )
{
	return cRequestedPowerDbm > cMaxTxPower ? cMaxTxPower : cRequestedPowerDbm;
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void prvBtAppTask( void *pvParameters )
{
	UNUSED( pvParameters );
	for ( ;; ) {
		xEventGroupWaitBits( xBtEventHandle,
							 BLUETOOTH_EVENT_FLAG_EVT_WAITING,
							 BLUETOOTH_EVENT_FLAG_EVT_WAITING, // Was UINT32_MAX
							 pdFALSE,
							 portMAX_DELAY );

		const uint32_t ulEventId = BGLIB_MSG_ID( bluetooth_evt->header );
		/* Special Case the Boot Event */
		if ( ulEventId == gecko_evt_system_boot_id ) {
			xTaskNotifyGive( xInitHandle );
		}
		/* Call general handlers */
		COMPILER_WARNING_DISABLE( "-Wcast-qual" )
		vBluetoothGapEventHandler( (struct gecko_cmd_packet *) bluetooth_evt );
		vBluetoothGattEventHandler( (struct gecko_cmd_packet *) bluetooth_evt );
		COMPILER_WARNING_ENABLE()
		xEventGroupSetBits( xBtEventHandle, BLUETOOTH_EVENT_FLAG_EVT_HANDLED );
	}
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void prvBtHostTask( void *pvParameters )
{
	EventBits_t flags = BLUETOOTH_EVENT_FLAG_EVT_HANDLED | BLUETOOTH_EVENT_FLAG_STACK;
	UNUSED( pvParameters );

	eLog( LOG_BLUETOOTH_GAP, LOG_DEBUG, "Host Starting\r\n" );
	for ( ;; ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_VERBOSE, "BT Host Loop 0x%X\r\n", flags );

		if ( flags & BLUETOOTH_EVENT_FLAG_CMD_WAITING ) {
			uint32_t		  header	  = command_header;
			gecko_cmd_handler cmd_handler = command_handler_func;
			sli_bt_cmd_handler_delegate_volatile( header, cmd_handler, command_data );
			command_handler_func = NULL;
			flags &= ~BLUETOOTH_EVENT_FLAG_CMD_WAITING;
			xEventGroupSetBits( xBtEventHandle, BLUETOOTH_EVENT_FLAG_RSP_WAITING );
		}

		//Bluetooth stack needs updating, and evt can be used
		if ( ( flags & BLUETOOTH_EVENT_FLAG_STACK ) && ( flags & BLUETOOTH_EVENT_FLAG_EVT_HANDLED ) ) { //update bluetooth & read event
			eLog( LOG_BLUETOOTH_GAP, LOG_VERBOSE, "BT EVT Received\r\n" );
			bluetooth_evt = gecko_peek_event();
			if ( bluetooth_evt != NULL ) { //we got event, notify event handler. evt state is now waiting handling
				xEventGroupSetBits( xBtEventHandle, BLUETOOTH_EVENT_FLAG_EVT_WAITING );
				flags &= ~( BLUETOOTH_EVENT_FLAG_EVT_HANDLED );
			}
			else { //nothing to do in stack, clear the flag
				flags &= ~( BLUETOOTH_EVENT_FLAG_STACK );
			}
		}

		// Ask from Bluetooth stack how long we can sleep
		// UINT32_MAX = sleep indefinitely
		// 0 = cannot sleep, stack needs update and we need to check if evt is handled that we can actually update it
		uint32_t timeout = gecko_can_sleep_ticks();
		if ( timeout == 0 && ( flags & BLUETOOTH_EVENT_FLAG_EVT_HANDLED ) ) {
			flags |= BLUETOOTH_EVENT_FLAG_STACK;
			continue;
		}
		eLog( LOG_BLUETOOTH_GAP, LOG_VERBOSE, "BT Host Sleep %u\r\n", timeout );

		if ( timeout == UINT32_MAX ) {
			timeout = portMAX_DELAY;
		}
		else {
			//round up to RTOS ticks
			timeout = ( timeout + BLUETOOTH_TO_RTOS_TICK - 1 ) / BLUETOOTH_TO_RTOS_TICK;
		}
		flags |= xEventGroupWaitBits( xBtEventHandle,
									  BLUETOOTH_EVENT_FLAG_STACK | BLUETOOTH_EVENT_FLAG_EVT_HANDLED | BLUETOOTH_EVENT_FLAG_CMD_WAITING,
									  BLUETOOTH_EVENT_FLAG_STACK | BLUETOOTH_EVENT_FLAG_EVT_HANDLED | BLUETOOTH_EVENT_FLAG_CMD_WAITING,
									  pdFALSE,
									  timeout );
		// Test for timeout by checking that none of the bits were set
		if ( !( flags & ( BLUETOOTH_EVENT_FLAG_STACK | BLUETOOTH_EVENT_FLAG_EVT_HANDLED | BLUETOOTH_EVENT_FLAG_CMD_WAITING ) ) ) {
			eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT Timeout\r\n" );
			flags |= BLUETOOTH_EVENT_FLAG_STACK;
		}
	}
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void prvBtLinkLayerTask( void *pvParameters )
{
	UNUSED( pvParameters );
	eLog( LOG_BLUETOOTH_GAP, LOG_DEBUG, "LL Starting\r\n" );
	for ( ;; ) {
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		eLog( LOG_BLUETOOTH_GAP, LOG_VERBOSE, "LL\r\n" );

		gecko_priority_handle();
	}
}

/*-----------------------------------------------------------*/

//This callback is called from interrupt context (Kernel Aware)
//sets flag to trigger Link Layer Task
static void vBluetoothLLCallback( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	// Notify the Link Layer task that it needs an update
	xTaskNotifyFromISR( pxBtLinkLayerHandle,
						BLUETOOTH_EVENT_FLAG_LL,
						eSetBits,
						&xHigherPriorityTaskWoken );

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

//This callback is called from Bluetooth stack
//Called from kernel aware interrupt context (RTCC interrupt) and from Bluetooth task
//sets flag to trigger running Bluetooth stack
static void vBluetoothUpdate( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	// Notify the Host task that it needs an update
	xEventGroupSetBitsFromISR( xBtEventHandle, BLUETOOTH_EVENT_FLAG_STACK, &xHigherPriorityTaskWoken );

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

void vBluetoothPend( void )
{
	xSemaphoreTake( xBtMutexHandle, portMAX_DELAY );
}

/*-----------------------------------------------------------*/

void vBluetoothPost( void )
{
	xSemaphoreGive( xBtMutexHandle );
}

/*-----------------------------------------------------------*/

COMPILER_WARNING_DISABLE( "-Wcast-qual" )
inline static void sli_bt_cmd_handler_delegate_volatile( uint32_t header, void ( *cmd_handler )( const void * ), volatile void *command_data )
{
	sli_bt_cmd_handler_delegate( header, cmd_handler, (void *) command_data );
}
COMPILER_WARNING_ENABLE()

/*-----------------------------------------------------------*/

void sli_bt_cmd_handler_rtos_delegate( uint32_t header, gecko_cmd_handler handler, const void *payload )
{
	eLog( LOG_BLUETOOTH_GAP, LOG_VERBOSE, "RTOS Delegate\r\n" );
	vBluetoothPend();
	command_header		 = header;
	command_handler_func = handler;
	COMPILER_WARNING_DISABLE( "-Wcast-qual" )
	command_data = (volatile void *) payload;
	COMPILER_WARNING_ENABLE()
	//Command structure is filled, notify the stack
	xEventGroupSetBits( xBtEventHandle, BLUETOOTH_EVENT_FLAG_CMD_WAITING );
	//wait for response
	xEventGroupWaitBits( xBtEventHandle,
						 BLUETOOTH_EVENT_FLAG_RSP_WAITING,
						 BLUETOOTH_EVENT_FLAG_RSP_WAITING, // Was UINT32_MAX
						 pdFALSE,
						 portMAX_DELAY );
	vBluetoothPost();
}

/*-----------------------------------------------------------*/
