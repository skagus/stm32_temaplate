#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_gpio.h>
#include "types.h"
#include "macro.h"
#include "sched.h"
#include "tick.h"
#include "led.h"
#include "led_matrix.h"

#define GPIO_HW_SPI			 			GPIOA
#define GPIO_HW_SPI_CS					GPIO_Pin_4    // NSS PIN
#define GPIO_HW_SPI_SCLK				GPIO_Pin_5    // Clock
#define GPIO_HW_SPI_MISO				GPIO_Pin_7    // Master Input to Slave Output

#define EN_NSS_PIN						(0)	// Not working..

void SPI_Init4led()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

		//	GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);

	GPIO_SetBits(GPIO_HW_SPI, GPIO_HW_SPI_MISO | GPIO_HW_SPI_SCLK | GPIO_HW_SPI_CS);

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = GPIO_HW_SPI_MISO | GPIO_HW_SPI_SCLK;
	stGpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIO_HW_SPI, &stGpioInit);

#if EN_NSS_PIN
	stGpioInit.GPIO_Pin = GPIO_HW_SPI_CS;
	stGpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIO_HW_SPI, &stGpioInit);
#else
	stGpioInit.GPIO_Pin = GPIO_HW_SPI_CS;
	stGpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIO_HW_SPI, &stGpioInit);
#endif

	//	GPIO_PinLockConfig(GPIO_HW_SPI, GPIO_HW_SPI_MISO | GPIO_HW_SPI_SCLK | GPIO_HW_SPI_CS);
	SPI_InitTypeDef stInitSpi;
	SPI_StructInit(&stInitSpi);
	stInitSpi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
	stInitSpi.SPI_Mode = SPI_Mode_Master;
	stInitSpi.SPI_DataSize = SPI_DataSize_16b;
#if EN_NSS_PIN
	stInitSpi.SPI_NSS = SPI_NSS_Hard;
#else
	stInitSpi.SPI_NSS = SPI_NSS_Soft;
#endif
	SPI_Init(SPI1, &stInitSpi);
	SPI_SSOutputCmd(SPI1, ENABLE);
	SPI_Cmd(SPI1, ENABLE);
}

#if EN_NSS_PIN
#define HW_SPI_CS_LOW()
#define HW_SPI_CS_HIGH()
#else
#define HW_SPI_CS_LOW()       GPIO_ResetBits(GPIO_HW_SPI, GPIO_HW_SPI_CS)
#define HW_SPI_CS_HIGH()      GPIO_SetBits(GPIO_HW_SPI, GPIO_HW_SPI_CS)
#endif

uint16 SPI_Tx(uint16 nData)
{
	uint16 nRcv = SPI_I2S_ReceiveData(SPI1);
	/* Send SPIy data */
	SPI_I2S_SendData(SPI1, nData);
	/* Wait for SPIy Tx buffer empty */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

	return nRcv;
}


#define DECODE_MODE_ADDR  (0x09)
#define DECODE_MODE_VAL   (0x00) // No decode.
#define INTENSITY_ADDR    (0x0A)
#define INTENSITY_VAL     (0x00) // Bright Setting (0x00 ~ 0x0F)
#define SCAN_LIMIT_ADDR   (0x0B)
#define SCAN_LIMIT_VAL    (0x07) // How many use digit led (line count in matrix led device)
#define POWER_DOWN_MODE_ADDR  (0x0C)
#define POWER_DOWN_MODE_VAR   (0x01) // Normal Op.(0x01)
#define TEST_DISPLAY_ADDR (0x0F)
#define TEST_DISPLAY_VAL  (0x00)

void SendMulti(uint8* aData, uint32 nLen)
{
	HW_SPI_CS_LOW();
	while (nLen)
	{
		SPI_Tx(*aData);
		aData++;
		nLen--;
	}
	HW_SPI_CS_HIGH();
}

void SendU16(uint16 nData)
{
	HW_SPI_CS_LOW();
	SPI_Tx(nData);
	HW_SPI_CS_HIGH();
}

void SendTwo(uint8 nAddr, uint8 nData)
{
	HW_SPI_CS_LOW();
	SPI_Tx(((uint16)nAddr << 8) | nData);
	HW_SPI_CS_HIGH();
}

void Matrix_Init()
{
	SendTwo(DECODE_MODE_ADDR, DECODE_MODE_VAL);
	SendTwo(INTENSITY_ADDR, INTENSITY_VAL);
	SendTwo(SCAN_LIMIT_ADDR, SCAN_LIMIT_VAL);
	SendTwo(POWER_DOWN_MODE_ADDR, POWER_DOWN_MODE_VAR);
	SendTwo(TEST_DISPLAY_ADDR, TEST_DISPLAY_VAL);
}

void temp(uint32 nCnt)
{
	uint16 aDspOdd[] = { 0x0101, 0x0203, 0x0307, 0x040F, 0x051F, 0x063F, 0x077F, 0x08FF };
	//	uint16 aDspEven[] = { 0x0155, 0x02AA, 0x0355, 0x04AA, 0x0555, 0x06AA, 0x0755, 0x08AA };
	uint16 aDspEven[] = { 0x0801, 0x0703, 0x0607, 0x050F, 0x041F, 0x033F, 0x027F, 0x01FF };

	if (nCnt & 0x1)
	{
		for (int i = 0; i < 8; i++)
		{
			SendU16(aDspOdd[i]);
		}
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			SendU16(aDspEven[i]);
		}
	}
}

void refreshMat(uint8* pFrame)
{
	for (int i = 0; i < 8; i++)
	{
		SendU16(((i + 1) << 8) | pFrame[i]);
	}
}

void ledmat_Run(void* pParam)
{
	static uint32 nCnt;

	static uint8 aDsp[8] = {};
	uint16 nCntMod = nCnt % 64;
	uint16 nCol = nCntMod % 8;
	uint16 nRow = nCntMod / 8;
	if (aDsp[nCol] & BIT(nRow))
	{
		BIT_CLR(aDsp[nCol], BIT(nRow));
	}
	else
	{
		BIT_SET(aDsp[nCol], BIT(nRow));
	}
	refreshMat(aDsp);

	nCnt++;
	Sched_Wait(0, SCHED_MSEC(10));
}

void LEDMat_Init()
{
	SPI_Init4led();
	Matrix_Init();

	Sched_Register(TID_LED_MAT, ledmat_Run, NULL, 0xFF);
}

