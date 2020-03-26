##############################################################################
# Verbosity control
##############################################################################

# Use  make V=1  to get verbose builds.
# TRACE_LD doesn't have additional $ because it is not called through eval
ifeq ($(V),1)
  APP_ROOT  := $(realpath .)
  REPO_ROOT := $(realpath $(APP_ROOT)/../..)
  TRACE_CC =
  TRACE_AS =
  TRACE_AR =
  TRACE_LD =
  Q=
else
  APP_ROOT  := .
  REPO_ROOT	:= $(APP_ROOT)/../..
  TRACE_CC = @echo "  CC       " $$<
  TRACE_AS = @echo "  AS       " $$<
  TRACE_AR = @echo "  AR       " $$@
  TRACE_LD = @echo "  LD       " $@
  Q=@
endif

#Define a newline variable for $(warning ) and $(error )
define n


endef

##############################################################################
# Command Validation
##############################################################################

# Sources and rules are split so test framework
# can pull sources and not pollute rules
    
# If we're trying to run tests
TEST_RULES := test test_clean coverage help clean clean_all test_python jlinksn
ifneq ($(filter $(MAKECMDGOALS),$(TEST_RULES)),)
# Set a dummy target
TARGET := nrf52840dk
# Otherwise, we're trying to compile an embedded application
else
# Check that the target we are requesting is supported by the application
ifeq ($(filter $(TARGET),$(SUPPORTED_TARGETS)),)  
    $(error Invalid target specified: "$(TARGET)" not in "$(SUPPORTED_TARGETS)")
endif
endif

# If we are performing a command that directly uses JLinkExe,
# validate that there is only one such device connected, or
# that a single device is specified via JLINK=
JLINK_RULES := gdb gdbserver jlink
ifneq ($(filter $(MAKECMDGOALS),$(JLINK_RULES)),)
ifndef JLINK
NUM_JLINK:=$(shell python3 -m programming.jlink --target $(TARGET) --print list | grep -c ".*")
ifneq ($(NUM_JLINK),$(filter $(NUM_JLINK),0 1))
$(error $n$nExpected to find less than 2 JLink devices, instead found $(NUM_JLINK) devices$n\
	Could not determine which device to use...$n\
	Connected devices can be listed via 'make jlinksn'$n\
	Device can be specified via 'make JLINK=1234567890 gdb'$n$n)
endif
endif
endif

# Determine if we are building in release or debug mode
DEBUG_RULES := debug flashdbg gdb gdbserver gdbrun echo
ifeq ($(filter $(MAKECMDGOALS),$(DEBUG_RULES)),)  
BUILD_MODE := REL
else
BUILD_MODE := DBG
endif

##############################################################################
# Base Variables
##############################################################################

# Flags passed to C compiler
CFLAGS            := $(APP_CFLAGS)
# Flags passed to final linker stage
LDFLAGS           = -L$(ARCH_LIB_DIR) -L$(PLATFORM_LIB_DIR)
# Flags passed to library archiver
ARFLAGS           := crT

# Externally provided libraries
EXTERNAL_LIBS     := -lm
# Libraries which always compile to the same output for a given architecture
ARCH_LIBS         :=
# Libraries which always compile to the same output for a given platform
PLATFORM_LIBS     := CORE_CSIRO

# For each library in $(ARCH_LIBS) and $(PLATFORM_LIBS), a corresponding $(LIB)_SRCS must exist
# $(LIB)_INCS and $(LIB)_SYS_INCS can also be defined for header search paths,  SYS_INCS are not error checked
# $(LIB)_CFLAGS can be used for library specific flags
CORE_CSIRO_SRCS   := 
CORE_CSIRO_INCS   :=
CORE_CSIRO_CFLAGS  = $(WARNINGS)

# Source files that are automatically generated 
GENERATED_SRCS    :=
# Source files to link as object files
APPLICATION_SRCS  := $(APPLICATION_SRCS)

##############################################################################
# Include Makefiles
##############################################################################

# Include definitions of all main project directories
include $(REPO_ROOT)/core_csiro/m_directories.mk

# Include target and architecture specific makefiles
include $(CORE_CSIRO_DIR)/platform/$(TARGET)/m_$(TARGET).mk
include $(CSIRO_ARCH_DIR)/m_$(CPU_ARCH).mk

# Optional CSIRO IP
-include $(CORE_CSIRO_DIR)/m_restricted.mk

# Include general sources and rules
include $(CORE_CSIRO_DIR)/m_sources.mk
include $(CORE_CSIRO_DIR)/m_rules.mk

##############################################################################
