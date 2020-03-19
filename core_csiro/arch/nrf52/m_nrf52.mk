##############################################################################
# Compiler Tools
##############################################################################

TOOLCHAIN := arm-none-eabi
CC        := $(TOOLCHAIN)-gcc
LD        := $(TOOLCHAIN)-gcc
AS        := $(TOOLCHAIN)-gcc
AR        := $(TOOLCHAIN)-ar
OBJCOPY   := $(TOOLCHAIN)-objcopy
SIZE      := $(TOOLCHAIN)-size
GDB       := $(TOOLCHAIN)-gdb
OBJDUMP   := $(TOOLCHAIN)-objdump
READELF   := $(TOOLCHAIN)-readelf

##############################################################################
# VSCode Options
##############################################################################

VSCODE_DEBUGGER	:= cortex-debug
SVD_OPTION 		:= --svd $(realpath $(CSIRO_ARCH_DIR)/cpu/$(CPU_VARIANT)/$(CPU_VARIANT).svd)

##############################################################################
# Architecture Directories
##############################################################################

NRF52_SDK_DIR 			:= $(CORE_EXTERNAL_DIR)/nrf52_sdk
SOFTWARE_CRC_DIR 		:= $(CORE_EXTERNAL_DIR)/software_crc

NRF_COMPONENTS_DIR  	:= $(NRF52_SDK_DIR)/components
NRF_EXTERNAL_DIR    	:= $(NRF52_SDK_DIR)/external
NRF_MODULES_DIR     	:= $(NRF52_SDK_DIR)/modules/nrfx
NRF_BLE_DIR         	:= $(NRF_COMPONENTS_DIR)/ble  
NRF_LIBRARIES_DIR   	:= $(NRF_COMPONENTS_DIR)/libraries

##############################################################################
# Architecture Defines
##############################################################################

CFLAGS  			+= -D$(CPU_VARIANT)=1
CFLAGS				+= -D__softdevice_binary__='"$(NRF_SOFTDEVICE_BIN)"'
CFLAGS 				+= -specs=nosys.specs

##############################################################################
# Architecture Specific Generated Files and Rules
##############################################################################

# Create softdevice .bin from .hex
$(NRF_SOFTDEVICE_BIN): $(NRF_SOFTDEVICE) | $(PLATFORM_LIB_DIR)
	$(Q)$(OBJCOPY) --gap-fill=255 -I ihex -O binary $^ $@

# gatt_nrf52.c is listed as a generated file, which are prerequisites for build rules
# By forcing the source to depend on the header, we can gaurantee the header is
# generated before other files are compiled
$(SRC_DIR)/gatt_nrf52.c : $(INC_DIR)/gatt_nrf52.h ;
$(INC_DIR)/gatt_nrf52.h : $(SRC_DIR)/gatt.xml
	$(Q)echo "  PY        Generating gatt_nrf52.{c,h}"
	$(Q)$(TOOLS_DIR)/autogen/generate_gatt.py --app_path $(APP_ROOT) --output_path_h $(INC_DIR) --output_path_c $(SRC_DIR)

GENERATED_SRCS 	+= $(SRC_DIR)/gatt_nrf52.c

##############################################################################
# SoC Specific Source Files
##############################################################################

APPLICATION_SRCS 	+= $(CORE_CSIRO_DIR)/arch/common/cpu/src/syscalls.c
APPLICATION_SRCS 	+= $(CORE_CSIRO_DIR)/arch/common/cpu/src/cortex_exceptions.c

##############################################################################
# SDK Library
##############################################################################

PLATFORM_LIBS 	+= NRF_SDK

# HAL sources
NRF_SDK_SRCS		:= $(SOFTWARE_CRC_DIR)/src/crc.c
NRF_SDK_SRCS		+= $(NRF_MODULES_DIR)/mdk/system_nrf52.c
NRF_SDK_SRCS    	+= $(NRF_MODULES_DIR)/drivers/src/nrfx_clock.c
NRF_SDK_SRCS    	+= $(NRF_MODULES_DIR)/drivers/src/nrfx_gpiote.c
NRF_SDK_SRCS    	+= $(NRF_MODULES_DIR)/drivers/src/nrfx_power.c
NRF_SDK_SRCS    	+= $(NRF_MODULES_DIR)/drivers/src/nrfx_ppi.c
NRF_SDK_SRCS    	+= $(NRF_MODULES_DIR)/drivers/src/nrfx_saadc.c
NRF_SDK_SRCS    	+= $(NRF_MODULES_DIR)/drivers/src/nrfx_systick.c
NRF_SDK_SRCS    	+= $(NRF_MODULES_DIR)/drivers/src/nrfx_twim.c
NRF_SDK_SRCS    	+= $(NRF_MODULES_DIR)/drivers/src/nrfx_wdt.c
NRF_SDK_SRCS    	+= $(wildcard $(NRF_MODULES_DIR)/drivers/src/prs/*.c)

NRF_SDK_SRCS    	+= $(NRF_MODULES_DIR)/soc/nrfx_atomic.c
NRF_SDK_SRCS    	+= $(NRF52_SDK_DIR)/integration/nrfx/legacy/nrf_drv_clock.c
NRF_SDK_SRCS    	+= $(NRF52_SDK_DIR)/integration/nrfx/legacy/nrf_drv_power.c
NRF_SDK_SRCS    	+= $(NRF_LIBRARIES_DIR)/fstorage/nrf_fstorage.c
NRF_SDK_SRCS    	+= $(NRF_LIBRARIES_DIR)/fstorage/nrf_fstorage_sd.c
NRF_SDK_SRCS    	+= $(NRF_LIBRARIES_DIR)/util/app_util_platform.c
NRF_SDK_SRCS    	+= $(wildcard $(NRF_LIBRARIES_DIR)/atomic/*.c)
NRF_SDK_SRCS    	+= $(wildcard $(NRF_LIBRARIES_DIR)/atomic_fifo/*.c)
NRF_SDK_SRCS    	+= $(wildcard $(NRF_LIBRARIES_DIR)/fds/*.c)
NRF_SDK_SRCS    	+= $(wildcard $(NRF_LIBRARIES_DIR)/fstorage/*.c)

# System includes (No Warnings)
NRF_SDK_SYS_INCS	:= $(NRF_COMPONENTS_DIR)
NRF_SDK_SYS_INCS  	+= $(NRF_MODULES_DIR)
NRF_SDK_SYS_INCS  	+= $(NRF_MODULES_DIR)/hal
NRF_SDK_SYS_INCS  	+= $(NRF_MODULES_DIR)/mdk
NRF_SDK_SYS_INCS  	+= $(NRF_MODULES_DIR)/drivers
NRF_SDK_SYS_INCS  	+= $(NRF_MODULES_DIR)/drivers/include
NRF_SDK_SYS_INCS  	+= $(NRF_MODULES_DIR)/drivers/src/prs
NRF_SDK_SYS_INCS  	+= $(NRF52_SDK_DIR)/integration/nrfx
NRF_SDK_SYS_INCS  	+= $(NRF52_SDK_DIR)/integration/nrfx/legacy
NRF_SDK_SYS_INCS  	+= $(NRF_SOFTDEVICE_DIR)/headers
NRF_SDK_SYS_INCS  	+= $(NRF_SOFTDEVICE_DIR)/headers/nrf52
NRF_SDK_SYS_INCS  	+= $(SEGGER_DIR)

NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/atomic
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/atomic_fifo
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/atomic_flags
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/balloc
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/crc16
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/delay
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/experimental_section_vars
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/fstorage
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/fds
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/log
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/log/src
NRF_SDK_SYS_INCS 	+= $(NRF_LIBRARIES_DIR)/pwm
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/strerror
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/timer
NRF_SDK_SYS_INCS  	+= $(NRF_LIBRARIES_DIR)/util

NRF_SDK_SYS_INCS  	+= $(NRF52_SDK_DIR)/components/ble/ble_advertising
NRF_SDK_SYS_INCS  	+= $(NRF52_SDK_DIR)/components/ble/common
NRF_SDK_SYS_INCS  	+= $(NRF52_SDK_DIR)/components/ble/nrf_ble_scan
NRF_SDK_SYS_INCS  	+= $(NRF52_SDK_DIR)/components/softdevice/common

NRF_SDK_SYS_INCS  	+= $(CSIRO_ARCH_DIR)/cpu/$(CPU_VARIANT)/inc

##############################################################################
# CMSIS
##############################################################################

ARCH_LIBS			+= CMSIS

CMSIS_SRCS			:= $(wildcard $(CMSIS_DSP_DIR)/Source/BasicMathFunctions/*.c)
CMSIS_SRCS			+= $(wildcard $(CMSIS_DSP_DIR)/Source/CommonTables/*.c)
CMSIS_SRCS			+= $(wildcard $(CMSIS_DSP_DIR)/Source/FastMathFunctions/*.c)

CMSIS_SYS_INCS		:= $(CMSIS_CORE_DIR)/Include
CMSIS_SYS_INCS		+= $(CMSIS_DSP_DIR)/Include

##############################################################################
# MBEDTLS
##############################################################################

ARCH_LIBS			+= MBEDTLS
CFLAGS				+= -DMBEDTLS_CONFIG_FILE="<mbedtls_config.h>"
CFLAGS				+= -DMBEDTLS_AES_ROM_TABLES
MBEDTLS_SRCS 		:= $(wildcard $(MBEDTLS_DIR)/library/*.c)
MBEDTLS_INCS 		:= $(CORE_EXTERNAL_DIR)/config/mbedtls
MBEDTLS_SYS_INCS 	:= $(MBEDTLS_DIR)/include

##############################################################################
# USBD
##############################################################################

NRF_USBD_SRCS		:= $(NRF_MODULES_DIR)/drivers/src/nrfx_usbd.c
NRF_USBD_SRCS		+= $(wildcard $(NRF52_SDK_DIR)/components/libraries/usbd/*.c)
NRF_USBD_SRCS		+= $(wildcard $(NRF52_SDK_DIR)/components/libraries/usbd/class/cdc/acm/*.c)
NRF_USBD_SRCS		+= $(CSIRO_ARCH_DIR)/interface/src/usb.c

NRF_USBD_SYS_INCS  	+= $(NRF52_SDK_DIR)/components/libraries/usbd
NRF_USBD_SYS_INCS  	+= $(NRF52_SDK_DIR)/components/libraries/usbd/class/cdc
NRF_USBD_SYS_INCS  	+= $(NRF52_SDK_DIR)/components/libraries/usbd/class/cdc/acm

##############################################################################
# Architecture Libraries and Linkers
##############################################################################

LDFLAGS		+= -L$(CSIRO_ARCH_DIR)/cpu
LDFLAGS 	+= -T$(wildcard $(CSIRO_ARCH_DIR)/cpu/$(CPU_VARIANT)/*.ld)

##############################################################################
