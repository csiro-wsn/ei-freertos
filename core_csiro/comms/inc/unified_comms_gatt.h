/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: unified_comms_gatt.h
 * Creation_Date: 23/08/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 *  CSIRO Unified Packet Interface for Bluetooth GATT
 * 
 * 	USER DATA ENCODING 
 *  https://confluence.csiro.au/display/pacp/On-Air+Packet+Format
 * 
 *  Relevant GATT Services
 *      9ac90001-c517-0d61-0c95-0d5593949597
 *          Primary Service
 * 
 *  Relevant GATT Characteristics
 *      9ac90002-c517-0d61-0c95-0d5593949597
 *          Input Data Characteristic
 *          Used for explicitly writing data to the GATT Server
 * 
 *      9ac90003-c517-0d61-0c95-0d5593949597
 *          Output Data Charactertistic
 *          Used for data responses that should be acknowledged
 * 
 *      9ac90004-c517-0d61-0c95-0d5593949597
 *          Output Data Characteristic
 *          Used for data responses that shouldn't be acknowledged
 */
#ifndef __CSIRO_CORE_UNIFIED_COMMS_GATT
#define __CSIRO_CORE_UNIFIED_COMMS_GATT
/* Includes -------------------------------------------------*/

#include "unified_comms.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eUnifiedCommsGattChannel_t {
	COMMS_CHANNEL_GATT_ACKED  = 0, /**< Overloads COMMS_CHANNEL_DEFAULT */
	COMMS_CHANNEL_GATT_NACKED = 1
} eUnifiedCommsGattChannel_t;

/* Variable Declarations ------------------------------------*/

/*
 * Bluetooth GATT Communication Interface
 */
extern xCommsInterface_t xGattComms;

/* Function Declarations ------------------------------------*/

#endif /* __CSIRO_CORE_UNIFIED_COMMS_GATT */
