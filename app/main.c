#include <stdint.h>
#include <stm32f10x.h>
#include <core_cm3.h>
#include <debug_cm3.h>
#include "sched.h"
#include "tick.h"
#include "console.h"
#include "led.h"

#define UNUSED(x)			(void)(x)

void assert_failed(uint8_t* file, uint32_t line)
{
	UNUSED(file);
	UNUSED(line);
}

extern const char* gpVersion;

int main(void)
{
	Cbf pfTickCb = Sched_Init();
	TICK_Init(MS_PER_TICK, pfTickCb);

	CON_Init();
	LED_Init();

	__enable_irq();
	CON_Printf("\nVER: %s\n", gpVersion);

#if 1
	Sched_Run();
#else
	int nCnt = gnLoop;
	for(;;)//int i=0; i< 10000; i++)
	{
		LED_Set(1);
		CON_Printf("On:  %4d, %8d\n", nCnt, nCntTick);
		TICK_Delay(TICK_PER_MS(1000));

		LED_Set(0);
		CON_Printf("Off: %4d\n", nCnt);
		TICK_Delay(TICK_PER_MS(1000));
		nCnt++;
	}
#endif
	while(1);

}

