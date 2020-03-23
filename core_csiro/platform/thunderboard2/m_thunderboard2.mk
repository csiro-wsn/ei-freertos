##############################################################################
# Platform CPU Selection
##############################################################################

PLATFORM_CPU	:= EFR32MG12P332F1024GL125
include $(CORE_CSIRO_DIR)/arch/EFR32/cpu/$(PLATFORM_CPU)/m_$(PLATFORM_CPU).mk

##############################################################################
# Platform Specific Peripheral Sources
##############################################################################

CORE_CSIRO_SRCS	:= $(CORE_CSIRO_DIR)/peripherals/memory/src/mx25r.c
CORE_CSIRO_SRCS	+= $(CORE_CSIRO_DIR)/peripherals/sensors/src/icm20648.c
CORE_CSIRO_SRCS	+= $(CORE_CSIRO_DIR)/peripherals/sensors/src/si1133.c

##############################################################################
# Platform Specific Task Sources
##############################################################################

##############################################################################
# Platform Specific Libraries
##############################################################################

##############################################################################