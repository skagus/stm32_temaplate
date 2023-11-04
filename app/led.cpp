#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include "types.h"
#include "os.h"


void LED_Set(int bOn)
{
	if (bOn)
	{
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
	}
	else
	{
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
	}
}

uint32 gbIsOn;
void led_Run(void* pParam)
{
	while (1)
	{
		if (gbIsOn)
		{
			LED_Set(1);
		}
		else
		{
			LED_Set(0);
		}
		gbIsOn = !gbIsOn;
		OS_Wait(0, OS_MSEC(100));
	}
}

#define SIZE_STK	(128)
static uint32 _aStk[SIZE_STK + 1];

void LED_Init()
{
	//// Enable GPIO C13.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // for LED.

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = GPIO_Pin_13;
	stGpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &stGpioInit);


	OS_CreateTask(led_Run, _aStk + SIZE_STK, NULL, "led");
}

