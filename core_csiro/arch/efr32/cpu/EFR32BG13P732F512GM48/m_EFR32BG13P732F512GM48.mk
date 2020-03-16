##############################################################################
# CPU Hardware Settings
##############################################################################

CPU_NAME			:= EFR32BG13P732F512GM48

CPU_CORTEX_FAMILY	:= ARM_CM4F
CPU_ARCH			:= EFR32
CPU_FAMILY			:= EFR32BG13P
CPU_VARIANT			:= $(CPU_NAME)

##############################################################################
# CPU Flags
##############################################################################

# Hardware Settings
CFLAGS     	+= -mlittle-endian -mthumb -mcpu=cortex-m4
CFLAGS     	+= -mfloat-abi=softfp -mfpu=fpv4-sp-d16
CFLAGS 		+= -DARM_MATH_CM4
CFLAGS 		+= -D__FPU_PRESENT

LDFLAGS    	+= -mlittle-endian -mthumb -mcpu=cortex-m4
LDFLAGS    	+= -mfloat-abi=softfp -mfpu=fpv4-sp-d16
LDFLAGS		+= -DARM_MATH_CM4
LDFLAGS		+= -D__FPU_PRESENT

##############################################################################
