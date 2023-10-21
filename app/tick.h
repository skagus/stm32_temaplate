#pragma once

#include <stdint.h>

typedef void (*Cbf)();

void TICK_Init(uint32_t nMsPerTick, Cbf cbfTick);
void TICK_Delay(uint32_t nTick);
