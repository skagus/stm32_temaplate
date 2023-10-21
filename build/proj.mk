
################ Top configuration.
PRJ_TOP = ..
TARGET = templ
OBJDIR = obj
OPTI = -O3

VER_STRING := $$(date +%Y%m%d_%H%M%S)_$(OPTI)

BUILD_STRICT = FALSE
BUILD_PRINT = FALSE

LINK_SCRIPT = startup/stm32f100.ld

################  Define
DEFINE = 
# STM32F103C8T6 <-- MD performance.
DEFINE += -DSTM32F10X_MD
DEFINE += -DUSE_STDPERIPH_DRIVER
DEFINE += -DUSE_FULL_ASSERT # ONLY for debug.
DEFINE += -DHSE_VALUE=8000000

################  Include.
# Add relative path from $(PRJ_TOP)
PRJ_INC = \
		cmsis \
		std_lib \
		app \
		utils

# Add absolue path. (ex. c:/lib/inc)
EXT_INC =

################  Library directory.
# Add relative path from $(PRJ_TOP)
PRJ_LIB_DIR =

# Add absolute path. (ex. c:/lib/)
EXT_LIB_DIR = 

LIB_FILE =

################ source files ##############
# Source file들은 project TOP 에서의 위치를 나타낸다.
CSRC =	\
		startup/startup_stm32f10x.c \
		cmsis/system_stm32f10x.c \
		cmsis/debug_cm3.c \
		std_lib/stm32f10x_usart.c \
		std_lib/stm32f10x_rcc.c \
		std_lib/stm32f10x_gpio.c \
		std_lib/misc.c \
		utils/sched.c \
		app/main.c \
		app/console.c \
		app/led.c \
		app/tick.c

CPPSRC =

ASRC =

DATA =

LINK_SCRIPT = startup/stm32f100.ld
