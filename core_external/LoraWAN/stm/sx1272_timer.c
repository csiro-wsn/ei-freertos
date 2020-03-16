#include "sx1272_timer.h"

BaseType_t xTimerHigherPriorityTaskWoken = pdFALSE;

inline uint8_t InInterrupt(void)
{
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

void TimerCallback (xTimerHandle TimerHandle)
{
    TimerEvent_t *Timer = (TimerEvent_t*)pvTimerGetTimerID(TimerHandle);
    Timer->Callback();
}

void TimerInit( TimerEvent_t *Obj, void ( *Callback )( void ) )
{
    Obj->Callback = Callback;
    Obj->TimerHandle = xTimerCreateStatic("T", pdMS_TO_TICKS(1000), pdFALSE, (void*)Obj, TimerCallback, &(Obj->TimerData));
}

void TimerStart( TimerEvent_t *Obj )
{
    if (InInterrupt()) {
        // ChangePeriod also starts the timer
        if (xTimerChangePeriodFromISR(Obj->TimerHandle, pdMS_TO_TICKS(Obj->ReloadValue), &xTimerHigherPriorityTaskWoken) != pdPASS) {
            while (1) ;
        }
    } else {
        xTimerChangePeriod(Obj->TimerHandle, pdMS_TO_TICKS(Obj->ReloadValue), 100);
    }
    
}

void TimerStop( TimerEvent_t *Obj )
{
    if (InInterrupt()) {
        xTimerStopFromISR(Obj->TimerHandle, &xTimerHigherPriorityTaskWoken);
    } else {
        xTimerStop(Obj->TimerHandle, 100);
    }
}

void TimerReset( TimerEvent_t *Obj )
{
    if (InInterrupt()) {
        xTimerResetFromISR(Obj->TimerHandle, &xTimerHigherPriorityTaskWoken);
    } else {
        xTimerReset(Obj->TimerHandle, 100);
    }
}

void TimerResetHigherPriorityWoken ( void )
{
    xTimerHigherPriorityTaskWoken = pdFALSE;
}

BaseType_t TimerHigherPriorityWoken ( void )
{
    return xTimerHigherPriorityTaskWoken;
}


void TimerSetValue( TimerEvent_t *Obj, uint32_t Value )
{
    Obj->ReloadValue = Value;
}

TimerTime_t TimerGetCurrentTime( void )
{
    return xTaskGetTickCount();
}

TimerTime_t TimerGetElapsedTime( TimerTime_t savedTime )
{
    return TimerGetCurrentTime() - savedTime;
}

TimerTime_t TimerGetFutureTime( TimerTime_t eventInFuture )
{
    return TimerGetCurrentTime() + eventInFuture;
}

void TimerLowPowerHandler( void )
{
    vTaskDelay(10);
    return;
}