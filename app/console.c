#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <stdarg.h>
#include <stdio.h>
#include <misc.h>
#include "sched_conf.h"
#include "sched.h"

#define MAX_BUF_SIZE		(128)

static char gaTxBuf[MAX_BUF_SIZE];

int CON_Puts(const char* szStr)
{
	while(*szStr)
	{
		while(RESET == USART_GetFlagStatus(USART1, USART_FLAG_TXE));
		USART_SendData(USART1, *szStr);
		szStr++;
	}
	return 0;
}

int CON_Printf(const char* szFmt, ...)
{
	va_list ap;
	int len;

	va_start(ap, szFmt);
	len = vsprintf(gaTxBuf, szFmt, ap);
	va_end(ap);

	CON_Puts(gaTxBuf);
	return len;
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		uint16_t nRxData = USART_ReceiveData(USART1);
		USART_SendData(USART1, (char)nRxData);
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}

uint32 gnCnt;
void con_Run(void* pParam)
{
	CON_Printf("On:  %4d\n", gnCnt);
	gnCnt++;

	Sched_Wait(0, SCHED_MSEC(1000));
}


void CON_Init()
{
	//// Enable GPIO A9, A10 for UART1.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = GPIO_Pin_9;
	stGpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
	stGpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &stGpioInit);

	stGpioInit.GPIO_Pin = GPIO_Pin_10;
	stGpioInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &stGpioInit);

	//// Setup UART1.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	USART_InitTypeDef stInitUart;
	USART_StructInit(&stInitUart);
	stInitUart.USART_BaudRate = 115200;
	USART_Init(USART1, &stInitUart);

	// setup UART Interrupt.
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_InitTypeDef stCfgNVIC;
	stCfgNVIC.NVIC_IRQChannel = USART1_IRQn;
	stCfgNVIC.NVIC_IRQChannelSubPriority = 8;
	stCfgNVIC.NVIC_IRQChannelPreemptionPriority = 8;
	stCfgNVIC.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&stCfgNVIC);

	USART_Cmd(USART1, ENABLE);



	Sched_Register(TID_ECHO, con_Run, NULL, 0xFF);
}
