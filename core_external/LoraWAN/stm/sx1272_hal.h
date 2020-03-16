#ifndef __SX1272_HAL_H
#define __SX1272_HAL_H

#include "stm32l4xx_hal.h"
#include "board.h"

#define RADIO_INIT_REGISTERS_VALUE                \
{                                                 \
    { MODEM_FSK , REG_LNA                , 0x23 },\
    { MODEM_FSK , REG_RXCONFIG           , 0x1E },\
    { MODEM_FSK , REG_RSSICONFIG         , 0xD2 },\
    { MODEM_FSK , REG_AFCFEI             , 0x01 },\
    { MODEM_FSK , REG_PREAMBLEDETECT     , 0xAA },\
    { MODEM_FSK , REG_OSC                , 0x07 },\
    { MODEM_FSK , REG_SYNCCONFIG         , 0x12 },\
    { MODEM_FSK , REG_SYNCVALUE1         , 0xC1 },\
    { MODEM_FSK , REG_SYNCVALUE2         , 0x94 },\
    { MODEM_FSK , REG_SYNCVALUE3         , 0xC1 },\
    { MODEM_FSK , REG_PACKETCONFIG1      , 0xD8 },\
    { MODEM_FSK , REG_FIFOTHRESH         , 0x8F },\
    { MODEM_FSK , REG_IMAGECAL           , 0x02 },\
    { MODEM_FSK , REG_DIOMAPPING1        , 0x00 },\
    { MODEM_FSK , REG_DIOMAPPING2        , 0x30 },\
    { MODEM_LORA, REG_LR_DETECTOPTIMIZE  , 0x43 },\
    { MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0x40 },\
}                                                 \

/************ SPI ****************/

void SX1272SpiInit(void);
void SX1272WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );
void SX1272ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );

/************ GPIO ****************/

void SX1272IoInit( void );
void SX1272IoDeInit( void );

void SX1272IoIrqInit( DioIrqHandler **irqHandlers );

void SX1272AssertReset(void);
void SX1272ReleaseReset(void);

/* Antenna Setting not supported on SX1272 board */
void SX1272SetAntSwLowPower( bool status );
void SX1272AntSwInit( void );
void SX1272AntSwDeInit( void );
void SX1272SetAntSw( uint8_t opMode );

/* All frequencies supported */
bool SX1272CheckRfFrequency( uint32_t frequency );

/************ General ****************/

void DelayMs(uint32_t Delay);
uint8_t SX1272DIO4IsConnected(void);

#endif /* __SX1272_HAL_H */