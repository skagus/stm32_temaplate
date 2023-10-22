#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_dma.h>
#include <stdarg.h>
#include <stdio.h>
#include <misc.h>
#include "macro.h"
#include "sched_conf.h"
#include "sched.h"


/**
 DMA 사용 전략.
	1. TX
		- Data는 TX buffer에 copy.
		- 현재 어디까지 valid한지, 
		- 어디까지 DMA를 걸어놨는지.
	2. RX
		- Buffer로 일단 RX 걸어 놓음 --> Half/Full ISR
		- Idle이면 ISR call.
*/

#define UART_TX_BUF_LEN			(128)
#define UART_RX_BUF_LEN			(64)

static uint8 gaTxBuf[UART_TX_BUF_LEN];
static uint8 gaRxBuf[UART_RX_BUF_LEN];


void DMA1_Channel5_IRQHandler(void)
{
	if(DMA_GetFlagStatus(DMA1_FLAG_HT5))
	{
		DMA_ClearFlag(DMA1_FLAG_HT5);
	}
	else if(DMA_GetFlagStatus(DMA1_FLAG_TE5))
	{
		DMA_ClearFlag(DMA1_FLAG_TE5);
	}
	else if(DMA_GetFlagStatus(DMA1_FLAG_TC5))
	{
		DMA_ClearFlag(DMA1_FLAG_TC5);
	}
	Sched_TrigAsyncEvt(BIT(EVT_UART_RX));
}

void USART1_IRQHandler(void)
{
	// 들어오다가, 멈추면 Interrupt발생.
	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
//		USART_ClearITPendingBit(USART1, USART_IT_IDLE);
		USART1->SR;
		USART1->DR;  /* clear USART_IT_IDLE bit */

		Sched_TrigAsyncEvt(BIT(EVT_UART_RX));
	}
}


void _DMA_Config(USART_TypeDef* macUSARTx)
{
	DMA_InitTypeDef DMA_InitStructure;

#if 0
	/* 
		part1: UART DMA TX config
	*/
	/* enable DMA clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	/* config DMA src */
	//DMA_InitStructure.DMA_PeripheralBaseAddr = USART1_DR_Base;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(& (macUSARTx->DR));

	/* config RAM addr. */
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)SendBuff;

	/* config direction */
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;

	/* config DMA_BufferSize */
	DMA_InitStructure.DMA_BufferSize = SENDBUFF_SIZE;

	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	/* DMA mode */
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal ;
	//DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;

	/* DMA_Priority */
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;

	/* DMA_M2M disable */
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	/* DMA1 channel4 */
	DMA_Init(macUSART_TX_DMA_CHANNEL, &DMA_InitStructure);

	/* enable DMA */
	DMA_Cmd(macUSART_TX_DMA_CHANNEL, ENABLE);

	/* DMA interrupt when tx done */
	//DMA_ITConfig(DMA1_Channel4,DMA_IT_TC,ENABLE);
#endif
	/*
		part2: UART DMA TX config
	*/
	/* enable DMA clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	/* DMA1 channel6 */
	DMA_DeInit(DMA1_Channel5);
	/* peri. addr */
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&macUSARTx->DR);
	/* DMA dir */
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	/* DMA RX buffer */
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)gaRxBuf;
	DMA_InitStructure.DMA_BufferSize = UART_RX_BUF_LEN;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;

	/* config DMA mode */
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	/* DMA_Priority */
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	DMA_ITConfig(DMA1_Channel5, DMA_IT_HT | DMA_IT_TC, ENABLE); 
	DMA_Cmd(DMA1_Channel5, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel    = DMA1_Channel5_IRQn; /* UART2 DMA1Rx*/      
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* 
		part3: UART interrupts
	*/
	USART_ITConfig(macUSARTx, USART_IT_TC, DISABLE);
	USART_ITConfig(macUSARTx, USART_IT_RXNE, DISABLE);
	USART_ITConfig(macUSARTx, USART_IT_IDLE, ENABLE);

#if 0
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif
}

//////////////////////////////////////////////////

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

uint32 UART_GetData(uint8* pBuf)
{
	static uint32 gnPrvIdx = 0;

	uint32 nCurIdx = UART_RX_BUF_LEN - DMA_GetCurrDataCounter(DMA1_Channel5);
	uint32 nCnt = 0;
	while(gnPrvIdx != nCurIdx)
	{
		*pBuf = gaRxBuf[gnPrvIdx];
		pBuf++;
		gnPrvIdx = (gnPrvIdx + 1) % UART_RX_BUF_LEN;
		nCnt++;
	}
	return nCnt;
}

/** 
Console Task.
*/
void con_Run(void* pParam)
{
	uint8 aBuf[128];
	uint32 nBytes = UART_GetData(aBuf);
	if(nBytes > 0)
	{
		aBuf[nBytes] = 0;
		CON_Printf("Cnt: %d, Rcv: %s\n", nBytes, aBuf);
	}

	Sched_Wait(BIT(EVT_UART_RX), SCHED_MSEC(10000));
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

	// UART DMA setup.
	_DMA_Config(USART1);
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

	USART_Cmd(USART1, ENABLE);


	Sched_Register(TID_ECHO, con_Run, NULL, 0xFF);
}
