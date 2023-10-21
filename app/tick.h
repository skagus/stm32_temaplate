#pragma once

#include <stdint.h>

void TICK_Init(uint32_t nMsPerTick);
uint32_t TICK_Get();
void TICK_Delay(uint32_t nTick);
