/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: unified_comms_serial.h
 * Creation_Date: 04/10/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * CSIRO Unified Packet Interface for Serial Comms
 * 
 * As a byte orientated interface which is also sending arbitrary text data,
 * the interface requires sync bytes to find packet starts and a length field
 * to know the complete length of the packet.
 * 
 *  RAW PACKET:		[ MAX 100 BYTES ]
 * 
 *  FIELDS:         [ 7 BYTE TRANSPORT HEADER ] [ MAX 93 BYTES PAYLOAD ]
 * 
 *  HEADER          [ 0xAA 0x55 ] [ PAYLOAD_LEN_LSB PAYLOAD_LEN_MSB ] [ ADDR_LSB ADDR_MSB ] [ SEQUENCE_NO ]
 * 
 */
#ifndef __CSIRO_CORE_COMMS_UNIFIED_SERIAL
#define __CSIRO_CORE_COMMS_UNIFIED_SERIAL

/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "unified_comms.h"

/* Module Defines -------------------------------------------*/

/* Type Definitions -----------------------------------------*/

/* Variable Declarations ------------------------------------*/

/*
 * Serial Communications Interface
 */
extern xCommsInterface_t xSerialComms;

/* Function Declarations ------------------------------------*/

void vSerialPacketBuilder( char cByte );

#endif /* __CSIRO_CORE_COMMS_UNIFIED_SERIAL */
