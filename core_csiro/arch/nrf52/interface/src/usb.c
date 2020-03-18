/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "usb.h"

#include "compiler_intrinsics.h"
#include "memory_pool.h"
#include "tiny_printf.h"

#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_core.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_string_desc.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define CDC_ACM_COMM_INTERFACE 		0
#define CDC_ACM_COMM_EPIN 			NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE 		1
#define CDC_ACM_DATA_EPIN 			NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT 			NRF_DRV_USBD_EPOUT1

#define USBD_STACK_SIZE   			256
#define USBD_PRIORITY   			2
#define USB_THREAD_MAX_BLOCK_TIME 	portMAX_DELAY

// clang-format on

/* Type Definitions -----------------------------------------*/

typedef struct xPendingTransmit_t
{
    uint8_t *pucBuffer;
    uint32_t ulBufferLen;
} xPendingTransmit_t;

/* Function Declarations ------------------------------------*/

static void vUsbdUserEventHandler(app_usbd_event_type_t event);
void vUsbdNewEventIsrHandler(app_usbd_internal_evt_t const *const p_event, bool queued);
static void vCdcAcmUserEventHandler(app_usbd_class_inst_t const *p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

void vUsbOn(void *pvContext);
void vUsbOff(void *pvContext);
eModuleError_t eUsbWrite(void *pvContext, const char *pcFormat, va_list pvArgs);
char *pcUsbClaimBuffer(void *pvContext, uint32_t *pulBufferLen);
void vUsbSendBuffer(void *pvContext, const char *pcBuffer, uint32_t ulBufferLen);
void vUsbReleaseBuffer(void *pvContext, char *pucBuffer);

static void vUsbdHandlerThread(void *pvParameters);

/* Private Variables ----------------------------------------*/

/* Private Variables ----------------------------------------*/

xSerialBackend_t xUsbBackend = {
    .fnEnable = vUsbOn,
    .fnDisable = vUsbOff,
    .fnWrite = eUsbWrite,
    .fnClaimBuffer = pcUsbClaimBuffer,
    .fnSendBuffer = vUsbSendBuffer,
    .fnReleaseBuffer = vUsbReleaseBuffer};

APP_USBD_CDC_ACM_GLOBAL_DEF(xAppCdcAcm,
                            vCdcAcmUserEventHandler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

#define TX_BUFFERS 4
#define READ_SIZE 1

static fnSerialByteHandler_t fnByteHandler = NULL;
static char pcRxBuffer[READ_SIZE];
// static char m_tx_buffer[NRF_DRV_USBD_EPSIZE];
// static bool m_send_flag = 0;

static TaskHandle_t xUsbdThread; /**< USB stack thread. */

/* Static memory pool as there can only be one SWD instance in a system */
MEMORY_POOL_CREATE(USB_POOL, TX_BUFFERS, SERIAL_INTERFACE_DEFAULT_SIZE);
static xMemoryPool_t *const pxUsbPool = &MEMORY_POOL_GET(USB_POOL);

/*-----------------------------------------------------------*/

void vUsbInit(void)
{
    vMemoryPoolInit(pxUsbPool);

    xTaskCreate(vUsbdHandlerThread, "USBD", USBD_STACK_SIZE, NULL, USBD_PRIORITY, &xUsbdThread);

    vTaskDelay(pdMS_TO_TICKS(10));
    /* Disable this IRQ so that Softdevice will init */
    NVIC_DisableIRQ(POWER_CLOCK_IRQn);
}

/*-----------------------------------------------------------*/

void vUsbSetByteHandler(fnSerialByteHandler_t fnHandler)
{
    fnByteHandler = fnHandler;
}

/*-----------------------------------------------------------*/

void vUsbOn(void *pvContext)
{
    UNUSED(pvContext);
}

/*-----------------------------------------------------------*/

void vUsbOff(void *pvContext)
{
    UNUSED(pvContext);
}

/*-----------------------------------------------------------*/

eModuleError_t eUsbWrite(void *pvContext, const char *pcFormat, va_list pvArgs)
{
    UNUSED(pvContext);
    char *pcBuffer = (char *)pcMemoryPoolClaim(pxUsbPool, portMAX_DELAY);
    uint32_t ulNumBytes = tiny_vsnprintf(pcBuffer, pxUsbPool->ulBufferSize, pcFormat, pvArgs);

    vUsbSendBuffer(pvContext, pcBuffer, ulNumBytes);
    return ERROR_NONE;
}

/*-----------------------------------------------------------*/

char *pcUsbClaimBuffer(void *pvContext, uint32_t *pulBufferLen)
{
    UNUSED(pvContext);
    *pulBufferLen = pxUsbPool->ulBufferSize;
    return (char *)pcMemoryPoolClaim(pxUsbPool, portMAX_DELAY);
}

/*-----------------------------------------------------------*/

void vUsbSendBuffer(void *pvContext, const char *pcBuffer, uint32_t ulBufferLen)
{
    UNUSED(pvContext);
    /* Send the data */
    app_usbd_cdc_acm_write(&xAppCdcAcm, pcBuffer, ulBufferLen);
    /* Release the buffer */
    vMemoryPoolRelease(pxUsbPool, (int8_t *)pcBuffer);
}

/*-----------------------------------------------------------*/

void vUsbReleaseBuffer(void *pvContext, char *pcBuffer)
{
    UNUSED(pvContext);
    vMemoryPoolRelease(pxUsbPool, (int8_t *)pcBuffer);
}

/*-----------------------------------------------------------*/

static void vCdcAcmUserEventHandler(app_usbd_class_inst_t const *p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    UNUSED(p_inst);

    switch (event)
    {
    case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
    {
        /*Setup first transfer*/
        ret_code_t ret = app_usbd_cdc_acm_read(&xAppCdcAcm,
                                               pcRxBuffer,
                                               READ_SIZE);
        UNUSED_VARIABLE(ret);
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
        break;
    case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
        break;
    case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
    {
        ret_code_t ret;
        do
        {
            /* Fetch data until internal buffer is empty */
            ret = app_usbd_cdc_acm_read(&xAppCdcAcm,
                                        pcRxBuffer,
                                        READ_SIZE);
            if ((ret == NRF_SUCCESS) && (fnByteHandler != NULL))
            {
                fnByteHandler(pcRxBuffer[0]);
            }
        } while (ret == NRF_SUCCESS);
        break;
    }
    default:
        break;
    }
}

/*-----------------------------------------------------------*/

static void vUsbdUserEventHandler(app_usbd_event_type_t event)
{
    switch (event)
    {
    case APP_USBD_EVT_DRV_SUSPEND:
        break;
    case APP_USBD_EVT_DRV_RESUME:
        break;
    case APP_USBD_EVT_STARTED:
        break;
    case APP_USBD_EVT_STOPPED:
        app_usbd_disable();
        break;
    case APP_USBD_EVT_POWER_DETECTED:
        if (!nrf_drv_usbd_is_enabled())
        {
            app_usbd_enable();
        }
        break;
    case APP_USBD_EVT_POWER_REMOVED:
        app_usbd_stop();
        break;
    case APP_USBD_EVT_POWER_READY:
        app_usbd_start();
        break;
    default:
        break;
    }
}

/*-----------------------------------------------------------*/

void vUsbdNewEventIsrHandler(app_usbd_internal_evt_t const *const p_event, bool queued)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    UNUSED_PARAMETER(p_event);
    UNUSED_PARAMETER(queued);
    ASSERT(xUsbdThread != NULL);
    /* Release the semaphore */
    vTaskNotifyGiveFromISR(xUsbdThread, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void vUsbdHandlerThread(void *arg)
{
    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_isr_handler = vUsbdNewEventIsrHandler,
        .ev_state_proc = vUsbdUserEventHandler};
    UNUSED(arg);

    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);
    app_usbd_class_inst_t const *class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&xAppCdcAcm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);
    ret = app_usbd_power_events_enable();
    APP_ERROR_CHECK(ret);

    // Set the first event to make sure that USB queue is processed after it is started
    UNUSED_RETURN_VALUE(xTaskNotifyGive(xTaskGetCurrentTaskHandle()));
    // Enter main loop.
    for (;;)
    {
        /* Waiting for event */
        UNUSED_RETURN_VALUE(ulTaskNotifyTake(pdTRUE, USB_THREAD_MAX_BLOCK_TIME));
        while (app_usbd_event_queue_process())
        {
            /* Nothing to do */
        }
    }
}

/*-----------------------------------------------------------*/
