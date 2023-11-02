#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_dma.h>
#include <stdarg.h>
#include <stdio.h>
#include "config.h"
#include "misc.h"
#include "types.h"
#include "macro.h"
#include "sched_conf.h"
#include "sched.h"
#include "print_queue.h"


/**
 DMA 사용 전략.
	1. RX
		- Buffer로 일단 RX 걸어 놓음 --> Half/Full ISR
		- Idle이면 ISR call --> RX Event trigger.
	2. TX
		- TX도 buffering이 필요함.
		- buffering은 printf에서 마지막으로 실행.
		- Buffer를 circular로 사용하도록.
		- circular시에 문제점은 sprintf를 하기가 곤란하다는 점이 있다.
		- 이를 좀 더 쉽게 풀기 위해서, 꼬리달린 circular 적용한다.
		- +-----------------#---------@
		- + ~ # 까지만 circular 로 운용하는데, sprintf 에서는 buffer 끝은 @ 까지 허용함.
		- 하지만, 새로운 print 시점에서 current buffer위치가 #를 넘어가지는 않도록.
		- print결과 #가 넘어간 경우에는, + 쪽으로 copy작업하여 circular 유지.
		- 즉, circular로 사용하되, sprintf를 간단히 하기 위해서 꼬리를 허용하는 개념임.
*/


#define UART_RX_BUF_LEN			(128)

volatile uint32 gbTxRun;
static uint8 gaRxBuf[UART_RX_BUF_LEN];

PrintBuf gstTxBuf;

void _UART_DMA_Tx();

FORCE_C void DMA1_Channel5_IRQHandler(void)
{
	if (DMA_GetFlagStatus(DMA1_FLAG_HT5))
	{
		DMA_ClearFlag(DMA1_FLAG_HT5);
	}
	else if (DMA_GetFlagStatus(DMA1_FLAG_TE5))
	{
		DMA_ClearFlag(DMA1_FLAG_TE5);
	}
	else if (DMA_GetFlagStatus(DMA1_FLAG_TC5))
	{
		DMA_ClearFlag(DMA1_FLAG_TC5);
	}
	Sched_TrigAsyncEvt(BIT(EVT_UART_RX));
}


FORCE_C void DMA1_Channel4_IRQHandler(void)
{
	if (DMA_GetFlagStatus(DMA1_FLAG_TC4))
	{
		DMA_ClearFlag(DMA1_FLAG_TC4);
		gstTxBuf.PQ_UpdateDelPtr();
		gbTxRun = 0;
		_UART_DMA_Tx();
	}
	Sched_TrigAsyncEvt(BIT(EVT_UART_TX));
}

FORCE_C void USART1_IRQHandler(void)
{
	// 들어오다가, 멈추면 Interrupt발생.
	if (USART_GetITStatus(CON_UART, USART_IT_IDLE) != RESET)
	{
		//		USART_ClearITPendingBit(CON_UART, USART_IT_IDLE);
		CON_UART->SR;
		CON_UART->DR;  /* clear USART_IT_IDLE bit */

		Sched_TrigAsyncEvt(BIT(EVT_UART_RX));
	}
}

void _UART_Config()
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

	NVIC_InitTypeDef stCfgNVIC;
	stCfgNVIC.NVIC_IRQChannel = USART1_IRQn;
	stCfgNVIC.NVIC_IRQChannelSubPriority = 8;
	stCfgNVIC.NVIC_IRQChannelPreemptionPriority = 8;
	stCfgNVIC.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&stCfgNVIC);
}

void _DMA_Tx_Config()
{
	DMA_InitTypeDef DMA_InitStructure;

	/* Config DMA basic */
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32)(&(CON_UART->DR));
	DMA_InitStructure.DMA_MemoryBaseAddr = 1;
	DMA_InitStructure.DMA_BufferSize = 1;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(CON_TX_DMA_CH, &DMA_InitStructure);
	DMA_ITConfig(CON_TX_DMA_CH, DMA_IT_TC, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void _DMA_Rx_Config()
{
	DMA_InitTypeDef DMA_InitStructure;

	//	UART DMA RX config
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&CON_UART->DR);
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)gaRxBuf;
	DMA_InitStructure.DMA_BufferSize = UART_RX_BUF_LEN;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  // NOT memory 2 memory.
	DMA_Init(CON_RX_DMA_CH, &DMA_InitStructure);
	DMA_ITConfig(CON_RX_DMA_CH, DMA_IT_HT | DMA_IT_TC, ENABLE);

	// DMA IRQ enable
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn; /* UART2 DMA1Rx*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Trigger UART RX DMA.
	DMA_Cmd(CON_RX_DMA_CH, ENABLE);

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

void _UART_DMA_Tx()
{
	if (0 == gbTxRun)
	{
		// while (RESET == USART_GetFlagStatus(CON_UART, USART_FLAG_TXE));
		DMA_Cmd(CON_TX_DMA_CH, DISABLE);

		uint32 nLen;
		uint8* pBuf = gstTxBuf.PQ_GetDelPtr(&nLen);
		if (nLen > 0)
		{
			CON_TX_DMA_CH->CPAR = (u32)(&(CON_UART->DR));  // Auto Cleared??
			CON_TX_DMA_CH->CMAR = (uint32)pBuf;
			CON_TX_DMA_CH->CNDTR = nLen;

			gbTxRun = 1;
			DMA_Cmd(CON_TX_DMA_CH, ENABLE);
		}
	}
}

//////////////////////////////////////////////////

int CON_Puts(const char* szStr)
{
	uint32 nBufLen;
	uint8* pBuf = gstTxBuf.PQ_GetAddPtr(&nBufLen);
	uint32 nStrLen = strlen(szStr);
	uint32 nCpyLen = MIN(nStrLen, nBufLen);
	memcpy(pBuf, szStr, nCpyLen);

	__disable_irq();
	gstTxBuf.PQ_UpdateAddPtr(nCpyLen);
	_UART_DMA_Tx();
	__enable_irq();

	return nCpyLen;
}

int CON_Printf(const char* szFmt, ...)
{
	uint32 nBufLen;
	uint8* pBuf = gstTxBuf.PQ_GetAddPtr(&nBufLen);

	va_list stVA;
	int nLen;

	va_start(stVA, szFmt);
	nLen = vsnprintf((char*)pBuf, nBufLen, szFmt, stVA);
	va_end(stVA);

	__disable_irq();
	gstTxBuf.PQ_UpdateAddPtr(nLen);
	_UART_DMA_Tx();
	__enable_irq();

	return nLen;
}

void CON_Flush()
{
	uint32 nAvail;
	do
	{
		gstTxBuf.PQ_GetAddPtr(&nAvail);
	} while (nAvail < 128);
}


uint32 UART_GetData(uint8* pBuf, uint32 nBufLen)
{
	static uint32 gnPrvIdx = 0;

	uint32 nCurIdx = UART_RX_BUF_LEN - DMA_GetCurrDataCounter(CON_RX_DMA_CH);
	uint32 nCnt = 0;
	while ((gnPrvIdx != nCurIdx) && (nCnt < nBufLen))
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
	uint32 nBytes = UART_GetData(aBuf, 128);
	if (nBytes > 0)
	{
		aBuf[nBytes] = 0;
		CON_Printf("%s", aBuf);
		for (uint32 nCnt = 0; nCnt < 1024; nCnt++)
		{
			if (0 == nCnt % 8)
			{
				CON_Printf("\n");
				CON_Flush();
			}
			CON_Printf("%8d ", nCnt);
		}
	}

	Sched_Wait(BIT(EVT_UART_RX), SCHED_MSEC(5000));
}


void CON_Init()
{
	_UART_Config();

	// for UART DMA, TC, RXNE is not used.
	//	USART_ITConfig(CON_UART, USART_IT_TC, DISABLE);
	//	USART_ITConfig(CON_UART, USART_IT_RXNE, DISABLE);
	USART_ITConfig(CON_UART, USART_IT_IDLE, ENABLE);

	/* enable DMA clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	_DMA_Tx_Config();
	_DMA_Rx_Config();

	USART_DMACmd(CON_UART, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE);
	USART_Cmd(CON_UART, ENABLE);

	Sched_Register(TID_ECHO, con_Run, NULL, 0xFF);
	//	gstTxBuf.Init();
}
