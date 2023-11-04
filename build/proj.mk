
################ Top configuration.
PRJ_TOP = ..
TARGET = out/templ
OBJDIR = out
OPTI = -O0

VER_STRING := $$(date +%Y%m%d_%H%M%S)_$(OPTI)

BUILD_STRICT = FALSE
BUILD_PRINT = TRUE

LINK_SCRIPT = startup/stm32f100.ld

################  Define
DEFINE = -DCPP_EXIST
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
		utils \
		startup

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
		std_lib/stm32f10x_usart.c \
		std_lib/stm32f10x_rcc.c \
		std_lib/stm32f10x_gpio.c \
		std_lib/stm32f10x_dma.c \
		std_lib/stm32f10x_spi.c \
		std_lib/misc.c \
		startup/startup_stm32f10x.c \
		utils/os.c \
		cmsis/system_stm32f10x.c \
		cmsis/debug_cm3.c

CPPSRC = \
		app/con_uart.cpp \
		app/console.cpp \
		app/led.cpp \
		app/tick.cpp \
		app/led_matrix.cpp \
		app/main.cpp \
		utils/print_queue.cpp

ASRC =

DATA =

LINK_SCRIPT = startup/stm32f100.ld
