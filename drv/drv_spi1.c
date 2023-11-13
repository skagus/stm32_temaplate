
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_dma.h>
#include "config.h"

#include "drv_spi1.h"

uint16 SPI1_Tx(uint16 nData)
{
	uint16 nRcv = SPI_I2S_ReceiveData(SPI1);
	/* Send SPIy data */
	SPI_I2S_SendData(SPI1, nData);
	/* Wait for SPIy Tx buffer empty */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

	return nRcv;
}

void SPI1_DmaTx(uint8* pBuf, uint16_t nLen)
{
	DMA_InitTypeDef stTxInit;
	DMA_StructInit(&stTxInit);
	stTxInit.DMA_DIR = DMA_DIR_PeripheralDST;
	stTxInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	stTxInit.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
	stTxInit.DMA_Priority = DMA_Priority_High;
	stTxInit.DMA_BufferSize = nLen;
	stTxInit.DMA_MemoryBaseAddr = (uint32)pBuf;
	DMA_Init(SPI1_TX_DMA_CH, &stTxInit);

	DMA_InitTypeDef stRxInit;
	DMA_StructInit(&stRxInit);
	stRxInit.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
	stRxInit.DMA_Priority = DMA_Priority_High;
	stRxInit.DMA_BufferSize = nLen;
	stRxInit.DMA_MemoryBaseAddr = (uint32)pBuf;
	stTxInit.DMA_DIR = DMA_DIR_PeripheralDST;
	stTxInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	stTxInit.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
	stTxInit.DMA_Priority = DMA_Priority_High;
	DMA_Init(SPI1_RX_DMA_CH, &stRxInit);
	/* Enable SPI Rx/Tx DMA Request*/

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, ENABLE);
	DMA_Cmd(SPI1_RX_DMA_CH, ENABLE);
	DMA_Cmd(SPI1_TX_DMA_CH, ENABLE);

	/* Waiting for the end of Data Transfer */
	while (DMA_GetFlagStatus(DMA1_FLAG_TC2) == RESET);
	while (DMA_GetFlagStatus(DMA1_FLAG_TC3) == RESET);

	DMA_ClearFlag(DMA1_FLAG_TC2 | DMA1_FLAG_TC3);

	DMA_Cmd(SPI1_RX_DMA_CH, DISABLE);
	DMA_Cmd(SPI1_TX_DMA_CH, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, DISABLE);
}

void SPI1_DMA_Init()
{
	// RX setup.
	DMA_InitTypeDef stRxInit;
	DMA_StructInit(&stRxInit);
	stRxInit.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
	stRxInit.DMA_Priority = DMA_Priority_High;
	DMA_Init(SPI1_RX_DMA_CH, &stRxInit);

	// TX Setup
	DMA_InitTypeDef stTxInit;
	DMA_StructInit(&stTxInit);
	stTxInit.DMA_DIR = DMA_DIR_PeripheralDST;
	stTxInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	stTxInit.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
	stTxInit.DMA_Priority = DMA_Priority_High;
	DMA_Init(SPI1_TX_DMA_CH, &stTxInit);
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
	stInitSpi.SPI_DataSize = SPI_DataSize_16b;
	stInitSpi.SPI_NSS = SPI_NSS_Soft;

	SPI_Init(SPI1, &stInitSpi);
	SPI_Cmd(SPI1, ENABLE);
}
