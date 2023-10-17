#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>

void LED_Init()
{
	//// Enable GPIO C13.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // for LED.

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = GPIO_Pin_13;
	stGpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC, &stGpioInit);
}


void LED_Set(int bOn)
{
	if(bOn)
	{
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
	}
	else
	{
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
	}
}


