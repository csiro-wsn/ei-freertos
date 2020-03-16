#include "sx1272_hal.h"
#include "interrupt_manager.h"

/**************** SPI ******************/

SPI_HandleTypeDef hspi1;

void SX1272SpiInit(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    HAL_SPI_Init(&hspi1);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    if (hspi->Instance == SPI1) {
        static GPIO_InitTypeDef Gpio_InitStruct;
        // NSS
        Gpio_InitStruct.Pin = SX1272_SPI_NSS_PIN;
        Gpio_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        Gpio_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(SX1272_SPI_NSS_PORT, &Gpio_InitStruct);
        HAL_GPIO_WritePin(SX1272_SPI_NSS_PORT, SX1272_SPI_NSS_PIN, 1);
        // MOSI
        Gpio_InitStruct.Pin = SX1272_SPI_MOSI_PIN;
        Gpio_InitStruct.Alternate = SX1272_SPI_MOSI_AF;
        Gpio_InitStruct.Mode = GPIO_MODE_AF_PP;
        Gpio_InitStruct.Pull = GPIO_PULLDOWN;
        HAL_GPIO_Init(SX1272_SPI_MOSI_PORT, &Gpio_InitStruct);
        // MISO
        Gpio_InitStruct.Pin = SX1272_SPI_MISO_PIN;
        Gpio_InitStruct.Alternate = SX1272_SPI_MISO_AF;
        Gpio_InitStruct.Mode = GPIO_MODE_AF_PP;
        Gpio_InitStruct.Pull = GPIO_PULLDOWN;
        HAL_GPIO_Init(SX1272_SPI_MISO_PORT, &Gpio_InitStruct);
        // SCK
        Gpio_InitStruct.Pin = SX1272_SPI_SCK_PIN;
        Gpio_InitStruct.Alternate = SX1272_SPI_SCK_AF;
        Gpio_InitStruct.Mode = GPIO_MODE_AF_PP;
        Gpio_InitStruct.Pull = GPIO_PULLDOWN;
        HAL_GPIO_Init(SX1272_SPI_SCK_PORT, &Gpio_InitStruct);
    }
}

void SX1272WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    // Assert Slave Select
    HAL_GPIO_WritePin(SX1272_SPI_NSS_PORT, SX1272_SPI_NSS_PIN, 0);
    // Set address
    addr |= 0x80;
    HAL_SPI_Transmit(&hspi1, &addr, 1, 100);
    // Transmit data
    HAL_SPI_Transmit(&hspi1, buffer, size, 100);
    // Release Slave Select
    HAL_GPIO_WritePin(SX1272_SPI_NSS_PORT, SX1272_SPI_NSS_PIN, 1);
}

void SX1272ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    // Assert Slave Select
    HAL_GPIO_WritePin(SX1272_SPI_NSS_PORT, SX1272_SPI_NSS_PIN, 0);
    // Set address
    addr &= 0x7F;
    HAL_SPI_Transmit(&hspi1, &addr, 1, 100);
    // Transmit data
    HAL_SPI_Receive(&hspi1, buffer, size, 100);
    // Release Slave Select
    HAL_GPIO_WritePin(SX1272_SPI_NSS_PORT, SX1272_SPI_NSS_PIN, 1);
}


/**************** GPIO ******************/

#define NUM_DIO     4
static uint32_t      const DIOPins[NUM_DIO] = {SX1272_DIO0_PIN, SX1272_DIO1_PIN, SX1272_DIO2_PIN, SX1272_DIO3_PIN};
static GPIO_TypeDef* const DIOPorts[NUM_DIO] = {SX1272_DIO0_PORT, SX1272_DIO1_PORT, SX1272_DIO2_PORT, SX1272_DIO3_PORT};
static uint32_t      const DIOIRQs[NUM_DIO] = {SX1272_DIO0_IRQn, SX1272_DIO1_IRQn, SX1272_DIO2_IRQn, SX1272_DIO3_IRQn};

void SX1272IoInit( void )
{
    static GPIO_InitTypeDef Gpio_InitStruct;
    uint32_t i;

    // Initialise DIO Pins
    Gpio_InitStruct.Mode = GPIO_MODE_INPUT;
    Gpio_InitStruct.Pull = GPIO_PULLUP;
    for (i = 0; i < NUM_DIO; i++) {
        Gpio_InitStruct.Pin = DIOPins[i];
        HAL_GPIO_Init(DIOPorts[i], &Gpio_InitStruct);
    }
}

void SX1272IoIrqInit( DioIrqHandler **irqHandlers )
{
    static GPIO_InitTypeDef Gpio_InitStruct;
    uint32_t i;

    // Initialise DIO Pins
    Gpio_InitStruct.Mode = GPIO_MODE_IT_RISING;
    Gpio_InitStruct.Pull = GPIO_PULLUP;
    for (i = 0; i < NUM_DIO; i++) {
        Gpio_InitStruct.Pin = DIOPins[i];
        HAL_GPIO_Init(DIOPorts[i], &Gpio_InitStruct);

        if (InterruptAddRTOS(DIOPins[i], DIOIRQs[i], irqHandlers[i], 5) < 0)
            while (1) ;
    }
}

void SX1272IoDeInit( void )
{
    static GPIO_InitTypeDef Gpio_InitStruct;
    uint32_t i;

    Gpio_InitStruct.Pin = SX1272_SPI_NSS_PIN;
    Gpio_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    Gpio_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SX1272_SPI_NSS_PORT, &Gpio_InitStruct);
    HAL_GPIO_WritePin(SX1272_SPI_NSS_PORT, SX1272_SPI_NSS_PIN, 1);

    // Initialise DIO Pins
    Gpio_InitStruct.Mode = GPIO_MODE_INPUT;
    Gpio_InitStruct.Pull = GPIO_NOPULL;
    for (i = 0; i < NUM_DIO; i++) {
        Gpio_InitStruct.Pin = DIOPins[i];
        HAL_GPIO_Init(DIOPorts[i], &Gpio_InitStruct);
    }
}

void SX1272AssertReset(void)
{
    static GPIO_InitTypeDef Gpio_InitStruct;
    // Configure Reset pin as floating input
    Gpio_InitStruct.Pin = SX1272_RESET_PIN;
    Gpio_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    Gpio_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SX1272_RESET_PORT, &Gpio_InitStruct);
    HAL_GPIO_WritePin(SX1272_RESET_PORT, SX1272_RESET_PIN, 1);
}

void SX1272ReleaseReset(void)
{
    static GPIO_InitTypeDef Gpio_InitStruct;
    // Configure Reset pin as floating input
    Gpio_InitStruct.Pin = SX1272_RESET_PIN;
    Gpio_InitStruct.Mode = GPIO_MODE_INPUT;
    Gpio_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SX1272_RESET_PORT, &Gpio_InitStruct);
}

/* Antenna Setting not supported on SX1272 board */
void SX1272SetAntSwLowPower( bool status ) {UNUSED(status); return;}
void SX1272AntSwInit( void ) {return;}
void SX1272AntSwDeInit( void ) {return;}
void SX1272SetAntSw( uint8_t opMode ) {UNUSED(opMode); return;}
/* All frequencies supported */
bool SX1272CheckRfFrequency( uint32_t frequency ) {UNUSED(frequency); return true;}

/******** General ********/

void DelayMs(uint32_t Delay)
{
    HAL_Delay(Delay);
}

uint8_t SX1272DIO4IsConnected(void) {return false;}
