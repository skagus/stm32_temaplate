
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_gpio.h>
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

#if 0 // FIXIT:
void SendMulti(uint8* aData, uint32 nLen)
{
	while (nLen)
	{
		SPI1_Tx(*aData);
		aData++;
		nLen--;
	}
}
#endif

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
	SPI_SSOutputCmd(SPI1, ENABLE);
	SPI_Cmd(SPI1, ENABLE);
}
