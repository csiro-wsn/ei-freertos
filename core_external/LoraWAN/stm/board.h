#ifndef __SX1272_BOARD_H
#define __SX1272_BOARD_H

#ifndef bool
    #define bool uint8_t
    #define true 1
    #define false 0
#endif

#include "stm32l4xx_hal.h"
#include "sx1272_config.h"

#include "sx1272_timer.h"
#include "radio.h"
#include "utilities.h"
#include "sx1272/sx1272.h"

#include "sx1272_hal.h"

#endif /* __SX1272_BOARD_H */