/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: application_defaults.h
 * Creation_Date: 17/09/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Application specific functions that can be implemented to handle common events
 * 
 */
#ifndef __CSIRO_CORE_APPLICATION_DEFAULTS
#define __CSIRO_CORE_APPLICATION_DEFAULTS
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "bluetooth.h"
#include "device_nvm.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void vApplicationSetLogLevels( void );

void vApplicationStartupCallback( void );

void vApplicationTickCallback( uint32_t ulUptime );

eModuleError_t eApplicationReconfigureFromNvm( eNvmKey_t eKey );

void vApplicationGattConnected( xBluetoothConnection_t *pxConnection );

void vApplicationGattDisconnected( xBluetoothConnection_t *pxConnection );

void vApplicationGattLocalCharacterisiticSubscribed( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxUpdatedCharacteristic );

void vApplicationGattLocalCharacterisiticWritten( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxUpdatedCharacteristic );

void vApplicationGattRemoteCharacterisiticChanged( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxUpdatedCharacteristic );

void vApplicationGattRemoteCharacterisiticRead( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxUpdatedCharacteristic );

#endif /* __CSIRO_CORE_APPLICATION_DEFAULTS */
