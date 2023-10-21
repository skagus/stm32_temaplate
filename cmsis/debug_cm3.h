#pragma once

#include <stm32f10x.h>
#include <core_cm3.h>

extern InterruptType_Type* gSCS;
extern SCB_Type* gSCB;
extern SysTick_Type* gSysTick;
extern NVIC_Type* gNVIC;
extern ITM_Type* gITM;
extern CoreDebug_Type* gCoreDebug;
