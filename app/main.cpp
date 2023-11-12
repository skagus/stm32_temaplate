#include <stdint.h>
#include <stm32f10x.h>
#include <core_cm3.h>
#include <debug_cm3.h>
#include "version.h"
#include "macro.h"
#include "os.h"
#include "tick.h"
#include "cli.h"
#include "led.h"
#include "led_matrix.h"

#define UNUSED(x)			(void)(x)

void assert_failed(uint8_t* file, uint32_t line)
{
	UNUSED(file);
	UNUSED(line);
	if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
	{
		__asm("bkpt 0");
	}
	else
	{
		DBG_TriggerNMI();
	}
}

FORCE_C int main(void)
{
	Cbf pfTickCb = OS_Init();
	TICK_Init(MS_PER_TICK, pfTickCb);

	CLI_Init();
	LED_Init();
	LEDMat_Init();

	__enable_irq();
	CLI_Printf("VER: %s\n", VERSION);

	OS_Start();

	while (1);
}

