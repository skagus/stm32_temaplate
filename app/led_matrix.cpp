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

#if 1
	stGpioInit.GPIO_Pin = GPIO_HW_SPI_CS;
	stGpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIO_HW_SPI, &stGpioInit);
#endif

	//	GPIO_PinLockConfig(GPIO_HW_SPI, GPIO_HW_SPI_MISO | GPIO_HW_SPI_SCLK | GPIO_HW_SPI_CS);
#if 1
	SPI_InitTypeDef stInitSpi;
	SPI_StructInit(&stInitSpi);
	stInitSpi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
	stInitSpi.SPI_Mode = SPI_Mode_Master;
	stInitSpi.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(SPI1, &stInitSpi);
	SPI_Cmd(SPI1, ENABLE);
#endif
}

/* Select hardware SPI: Chip Select pin low  */
#define HW_SPI_CS_LOW()       GPIO_ResetBits(GPIO_HW_SPI, GPIO_HW_SPI_CS)
/* Deselect hardware SPI: Chip Select pin high */
#define HW_SPI_CS_HIGH()      GPIO_SetBits(GPIO_HW_SPI, GPIO_HW_SPI_CS)


uint8 SPI_Tx(uint8 nData)
{
	uint8 nRcv = SPI_I2S_ReceiveData(SPI1);
	/* Send SPIy data */
	SPI_I2S_SendData(SPI1, nData);
	/* Wait for SPIy Tx buffer empty */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

	return nRcv;
}


#define DECODE_MODE_ADDR  (0x09)
#define DECODE_MODE_VAL   (0x00) // No decode.
#define INTENSITY_ADDR    (0x0A)
#define INTENSITY_VAL     (0x01) // Bright Setting (0x00 ~ 0x0F)
#define SCAN_LIMIT_ADDR   (0x0B)
#define SCAN_LIMIT_VAL    (0x07) // How many use digit led (line count in matrix led device)
#define POWER_DOWN_MODE_ADDR  (0x0C)
#define POWER_DOWN_MODE_VAR   (0x01) // Normal Op.(0x01)
#define TEST_DISPLAY_ADDR (0x0F)
#define TEST_DISPLAY_VAL  (0x00)

void SendTwo(uint8 nAddr, uint8 nData)
{
	HW_SPI_CS_LOW();
	SPI_Tx(nAddr);

	SPI_Tx(nData);
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

void ledmat_Run(void* pParam)
{
	const uint8 aDsp[] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA };
	static int nCnt;

	if (nCnt & 0x1)
	{
		for (int i = 0; i < 8; i++)
		{
			SendTwo(i + 1, aDsp[i]);
		}
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			SendTwo(i + 1, aDsp[i + 1]);
		}
	}

	nCnt++;
	Sched_Wait(0, SCHED_MSEC(1000));
}

void LEDMat_Init()
{
	SPI_Init4led();
	Matrix_Init();

	Sched_Register(TID_LED_MAT, ledmat_Run, NULL, 0xFF);
}

