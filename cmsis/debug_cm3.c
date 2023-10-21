#include <debug_cm3.h>

#define AT(x)	__attribute__((section(#x)))

ITM_Type* gITM AT(.dbg) = 			(ITM_Type*)ITM_BASE;
InterruptType_Type* gSCS AT(.dbg) = (InterruptType_Type*) SCS_BASE;
SysTick_Type* gSysTick AT(.dbg) = 	(SysTick_Type*)SysTick_BASE; 
NVIC_Type* gNVIC AT(.dbg) = 		(NVIC_Type*)NVIC_BASE;
SCB_Type* gSCB AT(.dbg) = 			(SCB_Type*)SCB_BASE;
CoreDebug_Type* gCORE_DBG AT(.dbg) = (CoreDebug_Type*)CoreDebug_BASE;
