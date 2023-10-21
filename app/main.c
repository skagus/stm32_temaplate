#include <stdint.h>
#include <stm32f10x.h>
#include <core_cm3.h>
#include <debug_cm3.h>
#include "tick.h"
#include "console.h"
#include "led.h"

#define UNUSED(x)			(void)(x)
#define MS_PER_TICK			(10)
#define TICK_PER_MS(nMS)	(nMS / MS_PER_TICK)

void assert_failed(uint8_t* file, uint32_t line)
{
	UNUSED(file);
	UNUSED(line);
}

int gnLoop = 100;
extern const char* gpVersion;
int main(void)
{
	CON_Init();
	LED_Init();
	TICK_Init(MS_PER_TICK);

	__enable_irq();

	CON_Printf("\nVER: %s\n", gpVersion);

	int nCnt = gnLoop;
	for(;;)//int i=0; i< 10000; i++)
	{
		LED_Set(1);
		CON_Printf("On:  %4d, %8d\n", nCnt, TICK_Get());
		TICK_Delay(TICK_PER_MS(1000));

		LED_Set(0);
		CON_Printf("Off: %4d\n", nCnt);
		TICK_Delay(TICK_PER_MS(1000));
		nCnt++;
	}
	while(1);
}

