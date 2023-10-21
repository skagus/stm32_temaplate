#include "stm32f10x.h"
#include "core_cm3.h"
#include "tick.h"

#define SYSCLK_FREQ 72000000

void _dummyCbf(){}

volatile uint32_t gnTick;
Cbf gfTick = _dummyCbf;

void TICK_Init(uint32_t nPeriodMs, Cbf cbfTick)
{
	SysTick_Config(SYSCLK_FREQ / 1000 * nPeriodMs);
	gfTick = cbfTick;
}

void TICK_Delay(uint32_t nTick)
{
	gnTick = 0;
	while(gnTick < nTick);
}

// SysTick exception handler.
void SysTick_Handler(void)
{
	gnTick++;
	gfTick();
}

