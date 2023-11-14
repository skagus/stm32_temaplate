#include <stm32f10x_gpio.h>
#include <stdio.h>
#include "types.h"
#include "config.h"
#include "macro.h"
#include "os.h"
#include "cli.h"
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

void SendMulti(uint8* pRxBuf, uint8* pTxBuf, uint16 nBytes)
{
	HW_SPI_CS_LOW();
#if 1
	SPI1_DmaTx(pRxBuf, pTxBuf, nBytes);
#else
	for (int i = 0; i < nBytes; i++)
	{
		pRxBuf[i] = SPI1_Tx(pTxBuf[i]);
	}
#endif
	HW_SPI_CS_HIGH();
}

void Matrix_Init()
{
	uint8 anInitPat[][2] =
	{
		{DECODE_MODE_ADDR, DECODE_MODE_VAL},
		{INTENSITY_ADDR, INTENSITY_VAL},
		{SCAN_LIMIT_ADDR, SCAN_LIMIT_VAL},
		{POWER_DOWN_MODE_ADDR, POWER_DOWN_MODE_VAR},
		{TEST_DISPLAY_ADDR, TEST_DISPLAY_VAL},
	};
	uint8 anRxBuf[10];

	uint32 nLoop = sizeof(anInitPat) / 2;
	for (int i = 0; i < nLoop; i++)
	{
		SendMulti(anRxBuf, anInitPat[i], 2);
	}
}

void refreshMat(uint8* pFrame)
{
	uint8 anTx[16];
	for (int i = 0; i < 8; i++)
	{
		anTx[i * 2] = 8 - i;
		anTx[i * 2 + 1] = pFrame[i];
	}

	uint8 anRx[16];
	for (int i = 0; i < 8; i++)
	{
		anRx[0] = 0xCC;
		SendMulti(anRx, anTx + (2 * i), 2);
#if 1
		char anBuf[20];
		sprintf(anBuf, "%X, %X -> %X %X\n",
			anTx[2 * i], anTx[2 * i + 1],
			anRx[0], anRx[1]);
		CLI_Puts(anBuf);
#endif
	}

#if 0
	CLI_Printf("T: %d\n", (int)10);
#elif 0
	CLI_Puts("TT\n");
#else
	//	CLI_Puts("TT--FF--FF--\n");
#endif
	OS_Idle(OS_MSEC(10));
}

void ledmat_Run(void* pParam)
{
	GPIO_InitTypeDef stGpioInit;
	GPIO_SetBits(LED_MAT_PORT, LED_MAT_CS);
	stGpioInit.GPIO_Pin = LED_MAT_CS;
	stGpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(LED_MAT_PORT, &stGpioInit);

	OS_Lock(BIT(LOCK_SPI1));
	Matrix_Init();
	OS_Unlock(BIT(LOCK_SPI1));

	uint8 nCnt = 0;
	uint8* aDsp = gaFont8x8[0];
	uint8 anIntensity[2] = { INTENSITY_ADDR , 0 };
	uint8 anDummy[2];
	while (1)
	{
		OS_Lock(BIT(LOCK_SPI1));

		refreshMat(aDsp);
		anIntensity[1] = nCnt / 2;
		SendMulti(anDummy, anIntensity, 2);

		OS_Unlock(BIT(LOCK_SPI1));

		aDsp = gaFont8x8[gnDspCh];
		OS_Wait(0, OS_MSEC(1000));
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

void ledmat_CmdDisp(uint8 argc, char* argv[])
{
	if (2 == argc)
	{
		gnDspCh = argv[1][0];
	}
	else
	{
		CLI_Printf("%s Display Char in LED matrix\r\n", argv[0]);
		CLI_Printf("\t$> %s <char> : Set display char\r\n", argv[0]);
	}
}

#define SIZE_STK	(128)
static uint32 _aStk[SIZE_STK + 1];

void LEDMat_Init()
{
	SPI1_Init();


	CLI_Register((const char*)"ledmat", ledmat_CmdDisp);

	OS_CreateTask(ledmat_Run, _aStk + SIZE_STK, NULL, "mat");
}

