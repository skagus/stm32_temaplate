#include "stm32f10x.h"
#include "core_cm3.h"
#include "tick.h"

volatile uint32_t gnTick;

#define SYSCLK_FREQ 72000000

uint32_t TICK_Get()
{
	return gnTick;
}

void TICK_Init(uint32_t nPeriodMs)
{
	gnTick = 0;
	SysTick_Config(SYSCLK_FREQ / 1000 * nPeriodMs);
}

void TICK_Delay(uint32_t nTick)
{
	uint32_t nStartTick = gnTick;
	while(gnTick < nStartTick + nTick);
}
// SysTick exception handler.
void SysTick_Handler(void)
{
	gnTick++;
}

