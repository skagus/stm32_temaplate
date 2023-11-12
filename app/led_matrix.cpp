#include <stm32f10x_gpio.h>
#include "types.h"
#include "config.h"
#include "macro.h"
#include "os.h"
#include "drv_spi1.h"
#include "led_matrix.h"

#define EN_NSS_PIN						(0)	// Not working..

extern uint8 gaFont8x8[128][8];
uint8 gnDspCh = 'A';

#define HW_SPI_CS_LOW()       GPIO_ResetBits(SPI1_PORT, LED_MAT_CS)
#define HW_SPI_CS_HIGH()      GPIO_SetBits(SPI1_PORT, LED_MAT_CS)

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

void SendTwo(uint8 nHigh, uint8 nLow)
{
	HW_SPI_CS_LOW();
	SPI1_Tx(((uint16)nHigh << 8) | nLow);
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
		SendTwo(8 - i, pFrame[i]);
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
	SPI1_Init();

	GPIO_InitTypeDef stGpioInit;
	GPIO_SetBits(LED_MAT_PORT, LED_MAT_CS);
	stGpioInit.GPIO_Pin = LED_MAT_CS;
	stGpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(LED_MAT_PORT, &stGpioInit);

	Matrix_Init();

	OS_CreateTask(ledmat_Run, _aStk + SIZE_STK, NULL, "mat");
}

