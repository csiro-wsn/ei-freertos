##############################################################################
# CPU Hardware Settings
##############################################################################

CPU_NAME			:= nrf52840

CPU_CORTEX_FAMILY	:= NRF52
CPU_ARCH			:= nrf52
CPU_VARIANT			:= $(CPU_NAME)

##############################################################################
# CPU Flags
##############################################################################

# Hardware Settings
CFLAGS     	+= -mlittle-endian -mthumb -mcpu=cortex-m4
CFLAGS     	+= -mfloat-abi=softfp -mfpu=fpv4-sp-d16
CFLAGS	 	+= -DARM_MATH_CM4
CFLAGS 		+= -D__FPU_PRESENT

LDFLAGS    	+= -mlittle-endian -mthumb -mcpu=cortex-m4
LDFLAGS    	+= -mfloat-abi=softfp -mfpu=fpv4-sp-d16
LDFLAGS 	+= -DARM_MATH_CM4
LDFLAGS		+= -D__FPU_PRESENT

# SDK and Softdevice
CFLAGS		+= -DNRF52840_XXAA
CFLAGS 		+= -DS140
CFLAGS 		+= -DNRF52_PAN_74
CFLAGS 		+= -DSWI_DISABLE0
CFLAGS 		+= -DSOFTDEVICE_PRESENT

##############################################################################
# NRF52840 Softdevice Sources
##############################################################################

NRF52_SDK_ROOT 					:= $(CORE_EXTERNAL_DIR)/nrf52_sdk
NRF_SOFTDEVICE_DIR  			:= $(NRF52_SDK_ROOT)/components/softdevice/s140
NRF_SOFTDEVICE  				:= $(NRF_SOFTDEVICE_DIR)/hex/s140_nrf52_6.1.1_softdevice.hex
NRF_SOFTDEVICE_BIN			   	:= $(PLATFORM_LIB_DIR)/$(notdir $(patsubst %.hex, %.bin, $(NRF_SOFTDEVICE)))

$(CSIRO_ARCH_DIR)/cpu/$(CPU_VARIANT)/src/gcc_startup_nrf52840.S: $(NRF_SOFTDEVICE_BIN)

##############################################################################
