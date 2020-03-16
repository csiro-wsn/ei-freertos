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

GECKO_SDK_DIR     	:= $(CORE_EXTERNAL_DIR)/gecko_sdk

SDK_BLUETOOTH_DIR	:= $(GECKO_SDK_DIR)/protocol/bluetooth
SDK_DEVICE_DIR		:= $(GECKO_SDK_DIR)/platform/Device/SiliconLabs/$(CPU_FAMILY)
SDK_EMLIB_DIR		:= $(GECKO_SDK_DIR)/platform/emlib
SDK_EMDRV_DIR		:= $(GECKO_SDK_DIR)/platform/emdrv

SDK_MBEDTLS_DIR		:= $(GECKO_SDK_DIR)/util/third_party/mbedtls

##############################################################################
# Architecture Defines
##############################################################################

CFLAGS  			+= -D$(CPU_FAMILY)
CFLAGS  			+= -D$(CPU_VARIANT)=1
CFLAGS 				+= -specs=nosys.specs

##############################################################################
# Architecture Specific Generated Files and Rules
##############################################################################

# gatt_efr32.c is listed as a generated file, which are prerequisites for build rules
# By forcing the source to depend on the header, we can gaurantee the header is
# generated before other files are compiled
$(SRC_DIR)/gatt_efr32.c : $(INC_DIR)/gatt_efr32.h ;
$(INC_DIR)/gatt_efr32.h : $(SRC_DIR)/gatt.xml
	$(Q)echo "  BGBUILD   Generating gatt_efr32.{c,h}"
	$(Q)bgbuild --gattonly $(SRC_DIR)/gatt.xml
	$(Q)rm -f $(SRC_DIR)/constants

GENERATED_SRCS 		+= $(SRC_DIR)/gatt_efr32.c

##############################################################################
# SoC Specific Source Files
##############################################################################

APPLICATION_SRCS 	+= $(CSIRO_ARCH_DIR)/FreeRTOS/src/LETIMER_tickless_idle.c
APPLICATION_SRCS 	+= $(CORE_CSIRO_DIR)/arch/common/cpu/src/syscalls.c
APPLICATION_SRCS 	+= $(CORE_CSIRO_DIR)/arch/common/cpu/src/cortex_exceptions.c

##############################################################################
# SDK Library
##############################################################################

PLATFORM_LIBS		+= EFR_SDK

# HAL sources
EFR_SDK_SRCS 		:= $(wildcard $(SDK_EMLIB_DIR)/src/*.c)
EFR_SDK_SRCS 		:= $(filter-out $(SDK_EMLIB_DIR)/src/em_int.c, $(EFR_SDK_SRCS))
EFR_SDK_SRCS 		+= $(wildcard $(SDK_EMDRV_DIR)/dmadrv/src/*.c)
EFR_SDK_SRCS 		+= $(wildcard $(SDK_EMDRV_DIR)/gpiointerrupt/src/*.c)
EFR_SDK_SRCS 		+= $(wildcard $(SDK_EMDRV_DIR)/nvm3/src/*.c)
EFR_SDK_SRCS 		+= $(wildcard $(SDK_EMDRV_DIR)/sleep/src/*.c)
EFR_SDK_SRCS 		+= $(wildcard $(SDK_EMDRV_DIR)/spidrv/src/*.c)
EFR_SDK_SRCS 		+= $(wildcard $(SDK_EMDRV_DIR)/tempdrv/src/*.c)
EFR_SDK_SRCS 		+= $(wildcard $(SDK_EMDRV_DIR)/uartdrv/src/*.c)
EFR_SDK_SRCS 		:= $(filter-out $(SDK_EMDRV_DIR)/nvm3/src/nvm3_default.c, $(EFR_SDK_SRCS))

# System includes (No Warnings, Not included in dependancy recompile checks)
EFR_SDK_SYS_INCS	:= $(SDK_BLUETOOTH_DIR)/ble_stack/inc/common
EFR_SDK_SYS_INCS	+= $(SDK_BLUETOOTH_DIR)/ble_stack/inc/soc
EFR_SDK_SYS_INCS	+= $(SDK_DEVICE_DIR)/Include
EFR_SDK_SYS_INCS	+= $(SDK_EMLIB_DIR)/inc
EFR_SDK_SYS_INCS	+= $(SDK_EMDRV_DIR)/common
EFR_SDK_SYS_INCS	+= $(GECKO_SDK_DIR)/platform/bootloader/api
EFR_SDK_SYS_INCS	+= $(wildcard $(SDK_EMDRV_DIR)/*/config)
EFR_SDK_SYS_INCS	+= $(wildcard $(SDK_EMDRV_DIR)/*/inc)

EFR_SDK_SYS_INCS	+= $(SDK_MBEDTLS_DIR)/include
EFR_SDK_SYS_INCS	+= $(SDK_MBEDTLS_DIR)/sl_crypto/include

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
CFLAGS				+= -DMBEDTLS_AES_ALT
CFLAGS				+= -DMBEDTLS_CONFIG_FILE="<mbedtls_config.h>"
MBEDTLS_SRCS 		:= $(wildcard $(MBEDTLS_DIR)/library/*.c)
MBEDTLS_SRCS 		+= $(wildcard $(SDK_MBEDTLS_DIR)/sl_crypto/src/*.c)
MBEDTLS_INCS 		:= $(CORE_EXTERNAL_DIR)/config/mbedtls
MBEDTLS_SYS_INCS 	:= $(MBEDTLS_DIR)/include
MBEDTLS_SYS_INCS	+= $(SDK_MBEDTLS_DIR)/sl_crypto/include

##############################################################################
# Architecture Libraries and Linkers
##############################################################################

LDFLAGS 			+= -T$(wildcard $(CSIRO_ARCH_DIR)/cpu/$(CPU_VARIANT)/*.ld)

LDFLAGS 			+= -L$(SDK_EMDRV_DIR)/nvm3/lib
LDFLAGS 			+= -L$(CSIRO_ARCH_DIR)/cpu
LDFLAGS 			+= -L$(SDK_BLUETOOTH_DIR)/lib/$(CPU_FAMILY)/$(COMPILER)

EXTERNAL_LIBS 		+= -lbluetooth
EXTERNAL_LIBS 		+= -lrail
EXTERNAL_LIBS 		+= -lnvm3_CM4_gcc


##############################################################################
