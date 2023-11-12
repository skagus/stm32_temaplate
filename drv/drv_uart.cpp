#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_dma.h>
#include "config.h"
#include "macro.h"
#include "misc.h"
#include "os.h"
#include "drv_uart.h"


volatile uint32 gbTxRun;
uint8 gaRxBuf[UART_RX_BUF_LEN];

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
	OS_AsyncEvt(BIT(EVT_UART_RX));
}


FORCE_C void DMA1_Channel4_IRQHandler(void)
{
	if (DMA_GetFlagStatus(DMA1_FLAG_TC4))
	{
		DMA_ClearFlag(DMA1_FLAG_TC4);
		gstTxBuf.PQ_UpdateDelPtr();
		gbTxRun = 0;
		UART_DmaTx();
	}
	OS_AsyncEvt(BIT(EVT_UART_TX));
}

FORCE_C void USART1_IRQHandler(void)
{
	// 들어오다가, 멈추면 Interrupt발생.
	if (USART_GetITStatus(CON_UART, USART_IT_IDLE) != RESET)
	{
		//		USART_ClearITPendingBit(CON_UART, USART_IT_IDLE);
		CON_UART->SR;
		CON_UART->DR;  /* clear USART_IT_IDLE bit */

		OS_AsyncEvt(BIT(EVT_UART_RX));
	}
}

void UART_Config()
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

	// for UART DMA, TC, RXNE is not used.
	//	USART_ITConfig(CON_UART, USART_IT_TC, DISABLE);
	//	USART_ITConfig(CON_UART, USART_IT_RXNE, DISABLE);
	USART_ITConfig(CON_UART, USART_IT_IDLE, ENABLE);

	USART_Cmd(CON_UART, ENABLE);
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
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
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

void UART_DmaTx()
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

void UART_DmaConfig()
{
	/* enable DMA clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	_DMA_Tx_Config();
	_DMA_Rx_Config();
	USART_DMACmd(CON_UART, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE);
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

uint8* UART_GetWrteBuf(uint32* pnLen)
{
	return gstTxBuf.PQ_GetAddPtr(pnLen);
}

void UART_PushWriteBuf(uint32 nLen)
{
	__disable_irq();
	gstTxBuf.PQ_UpdateAddPtr(nLen);
	UART_DmaTx();
	__enable_irq();
}

uint32 UART_CountTxFree()
{
	uint32 nAvail;
	gstTxBuf.PQ_GetAddPtr(&nAvail);
	return nAvail;
}
