#include <stdio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_gpio.h>

#include "types.h"
#include "macro.h"
#include "top_map.h"
#include "config.h"
#include <debug_cm3.h>

#define AT(x)	__attribute__((section(#x)))

ITM_Type* gITM AT(.dbg) = (ITM_Type*)ITM_BASE;
InterruptType_Type* gSCS AT(.dbg) = (InterruptType_Type*)SCS_BASE;
SysTick_Type* gSysTick AT(.dbg) = (SysTick_Type*)SysTick_BASE;
NVIC_Type* gNVIC AT(.dbg) = (NVIC_Type*)NVIC_BASE;
SCB_Type* gSCB AT(.dbg) = (SCB_Type*)SCB_BASE;
CoreDebug_Type* gCORE_DBG AT(.dbg) = (CoreDebug_Type*)CoreDebug_BASE;

void UrgentUartInit()
{
	//// Enable Clock for UART port and logic
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = GPIO_Pin_9;
	stGpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
	stGpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &stGpioInit);

	stGpioInit.GPIO_Pin = GPIO_Pin_10;
	stGpioInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &stGpioInit);

	USART_InitTypeDef stInitUart;
	USART_StructInit(&stInitUart);
	stInitUart.USART_BaudRate = 115200;
	USART_Init(CON_UART, &stInitUart);
	USART_Cmd(CON_UART, ENABLE);
}

void UrgentPrint(char* szString)
{
	while (*szString)
	{
		while (USART_GetFlagStatus(CON_UART, USART_FLAG_TXE) == RESET);
		USART_SendData(CON_UART, (uint16)(*szString));
		szString++;
	}
}

static void _DumpRegs(uint32* pStk)
{
	char aBuf[128];
	sprintf(aBuf, "In Urgent: Stk: 0x%08X\n", pStk);
	UrgentPrint(aBuf);
}

/**
 * This function should be called only in ISR.
*/
static void _DumpSRAM()
{
	uint32* pAddr = (uint32*)SRAM_BASE;
	char aBuf[128];
	while ((uint32)pAddr != SRAM_BASE + 1024)
	{
		if (0 == ((uint32)pAddr) % 32)
		{
			sprintf(aBuf, "\n%08X: ", pAddr);
			UrgentPrint(aBuf);
		}
		sprintf(aBuf, "%08X ", *pAddr);
		UrgentPrint(aBuf);
		pAddr++;
	}
}

void DumpAll(uint32* pStk)
{
	UrgentUartInit();
	_DumpRegs(pStk);
	_DumpSRAM();
}

__attribute__((naked)) FORCE_C void NMI_Handler(void)
{
	__asm("push {r0}");
	__asm("mov r0, pc");
	__asm("push {r1-r12}");
	__asm("mov r0, r13");
	__asm("bl DumpAll");
	while (1);
}

void DBG_TriggerNMI()
{
	SCB->ICSR |= SCB_ICSR_NMIPENDSET;
}
