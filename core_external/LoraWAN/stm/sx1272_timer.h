/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Timer Objects and scheduling management

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __SX1272_TIMER_H__
#define __SX1272_TIMER_H__

#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"


/*!
 * \brief Timer Object description
 */
typedef struct TimerEvent_s
{
    TimerHandle_t TimerHandle;
    StaticTimer_t TimerData;
    uint32_t ReloadValue;
    void (*Callback) (void);
}TimerEvent_t;

/*!
 * \brief Timer time variable definition
 */
#ifndef TimerTime_t
typedef uint32_t TimerTime_t;
#endif

/*!
 * \brief Initializes the timer Object
 *
 * \remark TimerSetValue function must be called before starting the timer.
 *         this function initializes timestamp and reload value at 0.
 *
 * \param [IN] Obj          Structure containing the timer Object parameters
 * \param [IN] callback     Function callback called at the end of the timeout
 */
void TimerInit( TimerEvent_t *Obj, void ( *Callback )( void ) );


/*!
 * \brief Starts and adds the timer Object to the list of timer events
 *
 * \param [IN] Obj Structure containing the timer Object parameters
 */
void TimerStart( TimerEvent_t *Obj );

/*!
 * \brief Stops and removes the timer Object from the list of timer events
 *
 * \param [IN] Obj Structure containing the timer Object parameters
 */
void TimerStop( TimerEvent_t *Obj );

/*!
 * \brief Resets the timer Object
 *
 * \param [IN] Obj Structure containing the timer Object parameters
 */
void TimerReset( TimerEvent_t *Obj );

// Timer Functions to call from Interrupts
void TimerResetHigherPriorityWoken ( void );
BaseType_t TimerHigherPriorityWoken ( void );

/*!
 * \brief Set timer new timeout value
 *
 * \param [IN] Obj   Structure containing the timer Object parameters
 * \param [IN] value New timer timeout value
 */
void TimerSetValue( TimerEvent_t *Obj, uint32_t Value );

/*!
 * \brief Read the current time
 *
 * \retval time returns current time
 */
TimerTime_t TimerGetCurrentTime( void );

/*!
 * \brief Return the Time elapsed since a fix moment in Time
 *
 * \param [IN] savedTime    fix moment in Time
 * \retval time             returns elapsed time
 */
TimerTime_t TimerGetElapsedTime( TimerTime_t savedTime );

/*!
 * \brief Return the Time elapsed since a fix moment in Time
 *
 * \param [IN] eventInFuture    fix moment in the future
 * \retval time             returns difference between now and future event
 */
TimerTime_t TimerGetFutureTime( TimerTime_t eventInFuture );

/*!
 * \brief Manages the entry into ARM cortex deep-sleep mode
 */
void TimerLowPowerHandler( void );

#endif  // __SX1272_TIMER_H__
