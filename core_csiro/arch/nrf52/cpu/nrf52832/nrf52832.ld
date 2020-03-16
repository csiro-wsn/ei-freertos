/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

/* 
/*   These values for softdevice memory lengths were from existing .ld files in the sdk,
/*   TODO: Check whether they are the smallest lengths that can be used ( Especially for RAM ).
/*    
*/

/* Memory Bases */
FLASH_BASE = 0x00;
RAM_BASE   = 0x20000000;

/* Device Specifics */
NRF52832_FLASH_LENGTH = 512k;
NRF52832_RAM_LENGTH   = 64K;

/* Softdevice memory */
SOFTDEVICE_FLASH_LENGTH = 0x26000;
SOFTDEVICE_RAM_LENGTH   = 0x04000;

/* NVM Size is defined by FDS_PHY_PAGES in fds_internal_defs.h */
NVM_LENGTH = 12K;

/* Setup memory region origins */
ORIGIN_SOFTDEVICE_FLASH   = FLASH_BASE;
ORIGIN_FLASH              = FLASH_BASE + SOFTDEVICE_FLASH_LENGTH; 
ORIGIN_NVM                = FLASH_BASE + NRF52832_FLASH_LENGTH - NVM_LENGTH;
ORIGIN_SOFTDEVICE_RAM     = RAM_BASE;
ORIGIN_RAM                = RAM_BASE + SOFTDEVICE_RAM_LENGTH;

/* Memory for application program */
MEMORY
{
  SOFTDEVICE_FLASH (rx) :  ORIGIN = ORIGIN_SOFTDEVICE_FLASH   LENGTH = SOFTDEVICE_FLASH_LENGTH
  FLASH (rx)            :  ORIGIN = ORIGIN_FLASH,             LENGTH = NRF52832_FLASH_LENGTH - SOFTDEVICE_FLASH_LENGTH - NVM_LENGTH
  NVM (r)               :  ORIGIN = ORIGIN_NVM,               LENGTH = NVM_LENGTH
  SOFTDEVICE_RAM (rwx)  :  ORIGIN = ORIGIN_SOFTDEVICE_RAM,    LENGTH = SOFTDEVICE_RAM_LENGTH
  RAM (rwx)             :  ORIGIN = ORIGIN_RAM,               LENGTH = NRF52832_RAM_LENGTH - SOFTDEVICE_RAM_LENGTH
}

INCLUDE "nrf52_common.ld"
