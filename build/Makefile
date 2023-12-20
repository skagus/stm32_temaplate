
################ Top configuration.
PRJ_TOP = ..
OBJDIR = out
TARGET = $(OBJDIR)/templ

OPTI = -O0
MCU = cortex-m3

VER_STRING := $$(date_time)_$(OPTI)

BUILD_STRICT = FALSE
BUILD_PRINT = FALSE

LINK_SCRIPT = startup/stm32f100.ld

################  Define
DEFINE = \
	CPP_EXIST \
	STM32F10X_MD \
	USE_STDPERIPH_DRIVER \
	USE_FULL_ASSERT \
	HSE_VALUE=8000000

# STM32F103C8T6 <-- MD performance.

################  Include.
# Add relative path from $(PRJ_TOP)
PRJ_INC = . \
		drv \
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
		drv/drv_spi1.c \
		std_lib/stm32f10x_usart.c \
		std_lib/stm32f10x_rcc.c \
		std_lib/stm32f10x_gpio.c \
		std_lib/stm32f10x_dma.c \
		std_lib/stm32f10x_spi.c \
		std_lib/misc.c \
		startup/startup_stm32f10x.c \
		cmsis/system_stm32f10x.c \
		cmsis/debug_cm3.c

CPPSRC = \
		app/spi_flash.cpp \
		app/font.cpp \
		app/cli.cpp \
		app/led.cpp \
		app/tick.cpp \
		app/led_matrix.cpp \
		app/main.cpp \
		drv/drv_uart.cpp \
		utils/os.cpp \
		utils/print.cpp \
		utils/print_queue.cpp

ASRC =

DATA =

########## TARGET #############
all: elf bin lst hex size

test:
	echo $(VER_STRING)

prog: $(TARGET).hex
	st-link_cli -P $(TARGET).hex -Rst
#	stm32_isp.exe stm32_conf.ini

run:
	st-link_cli -Rst

gdb: $(TARGET).elf
	$(GDB) $<


include rules.mk
