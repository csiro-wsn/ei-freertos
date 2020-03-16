Files from the Semtech LoraWAN "driver".


The following changes have been made to pull the code out of the application and into an actual driver.


######  sx1272.h ######

Removed Gpio_t and Spi_t members of SX1272_t struct

######  sx1272.c ######

Added Radio struct declaration from sx1272-board.c, as all functions are defined in this file, not sx1272-board.c.
Added SX1272SetRfTxPower() and SX1272GetPaSelect() implementation from sx1272-board.c

In SX1272Reset():
    Replaced GpioInit() calls with generic SX1272DriveReset() and SX1272ReleaseReset()

Remove implementation of SX1272WriteBuffer and SX1272WriteBuffer


In SX1272OnDio2Irq():
    Replace DIO4 connection check with call to SX1272DIO4IsConnected()