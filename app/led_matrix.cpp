#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_gpio.h>
#include "types.h"
#include "macro.h"
#include "os.h"
#include "tick.h"
#include "led.h"
#include "led_matrix.h"

#define GPIO_HW_SPI			 			GPIOA
#define GPIO_HW_SPI_CS					GPIO_Pin_4    // NSS PIN
#define GPIO_HW_SPI_SCLK				GPIO_Pin_5    // Clock
#define GPIO_HW_SPI_MISO				GPIO_Pin_7    // Master Input to Slave Output

#define EN_NSS_PIN						(0)	// Not working..

extern uint8 gaFont8x8[128][8];
uint8 gnDspCh;

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

void refreshMat(uint8* pFrame)
{
	for (int i = 0; i < 8; i++)
	{
		SendU16(((8 - i) << 8) | pFrame[i]);
	}
}

void ledmat_Run(void* pParam)
{
	uint8 nCnt = 0;
	uint8* aDsp = gaFont8x8[0];
	while (1)
	{
#if 0
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
		nCnt++;
#endif
		refreshMat(aDsp);
		SendTwo(INTENSITY_ADDR, nCnt / 2);

		aDsp = gaFont8x8[gnDspCh];
		OS_Wait(0, OS_MSEC(100));
		nCnt++;
		if (nCnt >= 8 * 2)
		{
			nCnt = 0;
		}
	}
}

void LEDMat_SendCh(char nCh)
{
	gnDspCh = nCh;
}

#define SIZE_STK	(128)
static uint32 _aStk[SIZE_STK + 1];

void LEDMat_Init()
{
	SPI_Init4led();
	Matrix_Init();

	OS_CreateTask(ledmat_Run, _aStk + SIZE_STK, NULL, "mat");
}

