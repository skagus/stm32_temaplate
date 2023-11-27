
#include <string.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_dma.h>
#include "config.h"

#include "drv_spi1.h"

DMA_InitTypeDef gstTxInit;
DMA_InitTypeDef gstRxInit;

uint8 SPI1_IO(uint8 nData)
{
	/* Send SPIy data */
	SPI_I2S_SendData(SPI1, nData);
	/* Wait for SPIy Tx buffer empty */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

	uint8 nRcv = SPI_I2S_ReceiveData(SPI1);

	return nRcv;
}


static uint8 saDummy[8];
void _DmaTrigger(uint8* pRx, uint8* pTx, uint16_t nLen)
{
	SPI_Cmd(SPI1, DISABLE);
	if (NULL != pTx)
	{
		//	gstTxInit.DMA_MemoryInc = bTx ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable;
		gstTxInit.DMA_Mode = DMA_Mode_Normal;
		gstTxInit.DMA_MemoryBaseAddr = (uint32)pTx;
		gstTxInit.DMA_BufferSize = nLen;
	}
	else
	{
		memset(saDummy, 0x00, sizeof(saDummy));
		gstTxInit.DMA_Mode = DMA_Mode_Circular;
		gstTxInit.DMA_MemoryBaseAddr = (uint32)saDummy;
		gstTxInit.DMA_BufferSize = 8;
	}
	DMA_Init(SPI1_TX_DMA_CH, &gstTxInit);

	//	gstRxInit.DMA_MemoryInc = bRx ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable;
	if (NULL != pRx)
	{
		gstRxInit.DMA_Mode = DMA_Mode_Normal;
		gstRxInit.DMA_MemoryBaseAddr = (uint32)pRx;
		gstRxInit.DMA_BufferSize = nLen;
	}
	else
	{
		memset(saDummy, 0x00, sizeof(saDummy));
		gstRxInit.DMA_Mode = DMA_Mode_Circular;
		gstRxInit.DMA_MemoryBaseAddr = (uint32)saDummy;
		gstRxInit.DMA_BufferSize = 8;
	}
	DMA_Init(SPI1_RX_DMA_CH, &gstRxInit);

	/* Enable SPI Rx/Tx DMA Request*/
	DMA_Cmd(SPI1_TX_DMA_CH, ENABLE);
	DMA_Cmd(SPI1_RX_DMA_CH, ENABLE);

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_Cmd(SPI1, ENABLE);


	/* Waiting for the end of Data Transfer */
	while (DMA_GetFlagStatus(DMA1_FLAG_TC2) == RESET); // DMA1 Channel 2.
	while (DMA_GetFlagStatus(DMA1_FLAG_TC3) == RESET); // DMA1 Channel 3.

	DMA_ClearFlag(0xF << 4); // GL2 | TC2 | HT2 | TE2
	DMA_ClearFlag(0xF << 8); // GL3 | TC3 | HT3 | TE3
	//	DMA_ClearFlag(DMA1_FLAG_TC2 | DMA1_FLAG_TC3);
	SPI_Cmd(SPI1, DISABLE);

	DMA_Cmd(SPI1_RX_DMA_CH, DISABLE);
	DMA_Cmd(SPI1_TX_DMA_CH, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_Cmd(SPI1, ENABLE);
}

void SPI1_DmaOut(uint8* pTx, uint16_t nLen)
{
	_DmaTrigger(NULL, pTx, nLen);
}

void SPI1_DmaIn(uint8* pRx, uint16_t nLen)
{
	_DmaTrigger(pRx, NULL, nLen);
}

void SPI1_DmaIO(uint8* pRx, uint8* pTx, uint16_t nLen)
{
	_DmaTrigger(pRx, pTx, nLen);
}

void SPI1_DMA_Init()
{
	// RX setup.
	DMA_StructInit(&gstRxInit);
	gstRxInit.DMA_MemoryBaseAddr = 1;
	gstRxInit.DMA_BufferSize = 1;
	gstRxInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	gstRxInit.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
	gstRxInit.DMA_Priority = DMA_Priority_High;
	//	DMA_Init(SPI1_RX_DMA_CH, &gstRxInit);

	// TX Setup
	DMA_StructInit(&gstTxInit);
	gstTxInit.DMA_MemoryBaseAddr = 1;
	gstTxInit.DMA_BufferSize = 1;
	gstTxInit.DMA_DIR = DMA_DIR_PeripheralDST;
	gstTxInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	gstTxInit.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
	gstTxInit.DMA_Priority = DMA_Priority_High;
	//	DMA_Init(SPI1_TX_DMA_CH, &gstTxInit);
}

void SPI1_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_SetBits(SPI1_PORT, SPI1_MOSI | SPI1_CLOCK);

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = SPI1_MOSI | SPI1_CLOCK;
	stGpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(SPI1_PORT, &stGpioInit);

	//	GPIO_PinLockConfig(GPIO_HW_SPI, GPIO_HW_SPI_MISO | GPIO_HW_SPI_SCLK | GPIO_HW_SPI_CS);
	SPI_InitTypeDef stInitSpi;
	SPI_StructInit(&stInitSpi);
	stInitSpi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
	stInitSpi.SPI_Mode = SPI_Mode_Master;
	stInitSpi.SPI_DataSize = SPI_DataSize_8b;
	stInitSpi.SPI_NSS = SPI_NSS_Soft;

	SPI_Init(SPI1, &stInitSpi);
#if 1
	SPI1_DMA_Init();
#endif
	SPI_Cmd(SPI1, ENABLE);
}
