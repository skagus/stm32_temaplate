#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_dma.h>
#include <stdarg.h>
#include <stdio.h>
#include "misc.h"
#include "types.h"
#include "macro.h"
#include "sched_conf.h"
#include "sched.h"


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

#define UART_TX_BUF_LEN			(512)
#define UART_TX_BUF_ADD			(64)
#define UART_RX_BUF_LEN			(128)

volatile uint32 gbTxRun;
static uint8 gaRxBuf[UART_RX_BUF_LEN];


struct _TxBuf
{
	uint16 nFree; ///< Free data.
	uint16 nValid; ///< DMA 대기 중인 data.
	uint16 nOnDMA; ///< DMA 중인 data, 0이 되면서 nNextFree, nFree 증가
	uint16 nNextAdd; ///< nFree를 감소.
	uint16 nNextFree; ///< 
	uint8 aBuf[UART_TX_BUF_LEN + UART_TX_BUF_ADD];
} gstTxBuf;

uint8* _GetNextAdd(struct _TxBuf* pTxBuf, uint32* pnMaxLen)
{
	uint32 nContLen = UART_TX_BUF_LEN + UART_TX_BUF_ADD - pTxBuf->nNextAdd;
	*pnMaxLen = (pTxBuf->nFree < nContLen) ? pTxBuf->nFree : nContLen;
	return pTxBuf->aBuf + pTxBuf->nNextAdd;
}

void _UpdateAdd(struct _TxBuf* pTxBuf, uint32 nLen)
{
	if (pTxBuf->nNextAdd + nLen > UART_TX_BUF_LEN)
	{
		uint32 nCpyLen = pTxBuf->nNextAdd + nLen - UART_TX_BUF_LEN;
		memcpy(pTxBuf->aBuf, pTxBuf->aBuf + UART_TX_BUF_LEN, nCpyLen);
	}
	pTxBuf->nValid += nLen;
	pTxBuf->nNextAdd = (pTxBuf->nNextAdd + nLen) % UART_TX_BUF_LEN;
	pTxBuf->nFree -= nLen;
}

/**
 * @return 남아 있는
*/
uint16 _DoneDMA(struct _TxBuf* pTxBuf)
{
	pTxBuf->nNextFree = (pTxBuf->nNextFree + pTxBuf->nOnDMA) % UART_TX_BUF_LEN;
	pTxBuf->nFree += pTxBuf->nOnDMA;
	pTxBuf->nValid -= pTxBuf->nOnDMA;
	pTxBuf->nOnDMA = 0;
	return pTxBuf->nValid;
}

uint8* _PopNextDMA(struct _TxBuf* pTxBuf, uint32* pnSize)
{
	uint8* pNxtDma = NULL;

	if (pTxBuf->nValid > 0)
	{
		if (pTxBuf->nNextFree + pTxBuf->nValid < UART_TX_BUF_LEN) // normal case.
		{
			*pnSize = pTxBuf->nValid;
			pNxtDma = pTxBuf->aBuf + pTxBuf->nNextFree;
		}
		else // roll over case.
		{
			*pnSize = UART_TX_BUF_LEN - pTxBuf->nNextFree;
			pNxtDma = pTxBuf->aBuf + pTxBuf->nNextFree;
		}
	}
	else
	{
		*pnSize = 0;
	}
	pTxBuf->nOnDMA = *pnSize;
	return pNxtDma;
}

void DMA1_Channel5_IRQHandler(void)
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


void DMA1_Channel4_IRQHandler(void)
{
	//	while (1);
	if (DMA_GetFlagStatus(DMA1_FLAG_TC4))
	{
		DMA_ClearFlag(DMA1_FLAG_TC4);
		_DoneDMA(&gstTxBuf);
		gbTxRun = 0;
		_UART_DMA_Tx();
	}
	Sched_TrigAsyncEvt(BIT(EVT_UART_TX));
}

void USART1_IRQHandler(void)
{
	// 들어오다가, 멈추면 Interrupt발생.
	if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		//		USART_ClearITPendingBit(USART1, USART_IT_IDLE);
		USART1->SR;
		USART1->DR;  /* clear USART_IT_IDLE bit */

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
	USART_Init(USART1, &stInitUart);

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
	DMA_InitStructure.DMA_PeripheralBaseAddr = 0; // (u32)(&(USART1->DR));
	DMA_InitStructure.DMA_MemoryBaseAddr = 0;
	DMA_InitStructure.DMA_BufferSize = 0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel4, &DMA_InitStructure);
	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void _DMA_Rx_Config()
{
	DMA_InitTypeDef DMA_InitStructure;

	//	UART DMA RX config
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);
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
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	DMA_ITConfig(DMA1_Channel5, DMA_IT_HT | DMA_IT_TC, ENABLE);

	// DMA IRQ enable
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn; /* UART2 DMA1Rx*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Trigger UART RX DMA.
	DMA_Cmd(DMA1_Channel5, ENABLE);

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
		// while (RESET == USART_GetFlagStatus(USART1, USART_FLAG_TXE));
		DMA_Cmd(DMA1_Channel4, DISABLE);

		uint32 nLen;
		uint8* pBuf = _PopNextDMA(&gstTxBuf, &nLen);
		if (nLen > 0)
		{
			DMA1_Channel4->CPAR = (u32)(&(USART1->DR));  // Auto Cleared??
			DMA1_Channel4->CMAR = pBuf;
			DMA1_Channel4->CNDTR = nLen;

			gbTxRun = 1;
			DMA_Cmd(DMA1_Channel4, ENABLE);
		}
	}
}

//////////////////////////////////////////////////

int CON_Printf(const char* szFmt, ...)
{
	uint32 nBufLen;
	uint8* pBuf = _GetNextAdd(&gstTxBuf, &nBufLen);

	va_list stVA;
	int nLen;

	va_start(stVA, szFmt);
	nLen = vsnprintf(pBuf, nBufLen, szFmt, stVA);
	va_end(stVA);

	__disable_irq();
	_UpdateAdd(&gstTxBuf, nLen);
	_UART_DMA_Tx();
	__enable_irq();

	return nLen;
}

int CON_Puts(const char* szStr)
{
	return CON_Printf(szStr);
}

uint32 UART_GetData(uint8* pBuf)
{
	static uint32 gnPrvIdx = 0;

	uint32 nCurIdx = UART_RX_BUF_LEN - DMA_GetCurrDataCounter(DMA1_Channel5);
	uint32 nCnt = 0;
	while (gnPrvIdx != nCurIdx)
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
	if (nBytes > 0)
	{
		aBuf[nBytes] = 0;
		CON_Printf("%s", aBuf);
	}

	Sched_Wait(BIT(EVT_UART_RX), SCHED_MSEC(10000));
}


void CON_Init()
{
	_UART_Config();

	// for UART DMA, TC, RXNE is not used.
	//	USART_ITConfig(USART1, USART_IT_TC, DISABLE);
	//	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

	/* enable DMA clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	_DMA_Tx_Config();
	_DMA_Rx_Config();

	USART_DMACmd(USART1, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE);
	USART_Cmd(USART1, ENABLE);

	Sched_Register(TID_ECHO, con_Run, NULL, 0xFF);
	gstTxBuf.nFree = UART_TX_BUF_LEN;
}
