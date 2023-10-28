#include "types.h"
#include "macro.h"
#include "stm32f10x.h"
#include "core_cm3.h"
#include "tick.h"

#define SYSCLK_FREQ 72000000

void _dummyCbf(uint32 tag, uint32 result) {}

volatile uint32_t gnTick;
Cbf gfTick = _dummyCbf;

void TICK_Init(uint32_t nPeriodMs, Cbf cbfTick)
{
	// stm32f103, feeds HCLK / 8 as external system timer clock. 
#if 1
	SysTick_Config(SYSCLK_FREQ / 1000 * nPeriodMs);
#else
	SysTick_Config(SYSCLK_FREQ / (8 * 1000) * nPeriodMs);
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
#endif
	gfTick = cbfTick;
}

void TICK_Delay(uint32_t nTick)
{
	gnTick = 0;
	while (gnTick < nTick);
}

// SysTick exception handler.
FORCE_C void SysTick_Handler(void)
{
	gnTick++;
	gfTick(0, 0);
}
