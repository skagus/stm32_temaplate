#include <stdint.h>
#include <stm32f10x.h>
#include <core_cm3.h>
#include <debug_cm3.h>
#include "macro.h"
#include "sched.h"
#include "tick.h"
#include "console.h"
#include "led.h"
#include "led_matrix.h"

#define UNUSED(x)			(void)(x)

void assert_failed(uint8_t* file, uint32_t line)
{
	UNUSED(file);
	UNUSED(line);
}

extern const char* gpVersion;

FORCE_C int main(void)
{
	Cbf pfTickCb = Sched_Init();
	TICK_Init(MS_PER_TICK, pfTickCb);

	CON_Init();
	LED_Init();
	LEDMat_Init();

	__enable_irq();
	CON_Puts("VER: ");
	CON_Puts(gpVersion);
	CON_Puts("\n");

	Sched_Run();

	while (1);
}

