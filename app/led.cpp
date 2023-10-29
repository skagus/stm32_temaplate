#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include "sched.h"


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
	if (gbIsOn)
	{
		LED_Set(1);
	}
	else
	{
		LED_Set(0);
	}
	gbIsOn = !gbIsOn;
	Sched_Wait(0, SCHED_MSEC(500));
}

void LED_Init()
{
	//// Enable GPIO C13.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // for LED.

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = GPIO_Pin_13;
	stGpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &stGpioInit);


	Sched_Register(TID_LED, led_Run, NULL, 0xFF);
}

