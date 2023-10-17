#include <stm32f10x_conf.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_rcc.h>

#define UNUSED(x)		(void)(x)

void assert_failed(uint8_t* file, uint32_t line)
{
	UNUSED(file);
	UNUSED(line);
}

void _mydelay(uint32_t nCycles)
{
	while(nCycles-- > 0)
	{
		__asm volatile("nop");
	}
}

void _send_char(USART_TypeDef* pPort, char nCh)
{
	while(0 == (pPort->SR & (1<<7)));
	USART_SendData(pPort, nCh);
}

void _send_string(USART_TypeDef* pPort, char* szStr)
{
	while(*szStr)
	{
		_send_char(pPort, *szStr);
		szStr++;
	}
}

void init_hw()
{
	//// Enable GPIO C13.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // for LED.

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = GPIO_Pin_13;
	stGpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC, &stGpioInit);

	//// Enable GPIO A9, A10 for UART1.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	stGpioInit.GPIO_Pin = GPIO_Pin_9;
	stGpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
	stGpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &stGpioInit);

	stGpioInit.GPIO_Pin = GPIO_Pin_10;
	stGpioInit.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &stGpioInit);

	//// Enable UART1.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	USART_InitTypeDef stInitUart;
	USART_StructInit(&stInitUart);
	stInitUart.USART_BaudRate = 115200;
	USART_Init(USART1, &stInitUart);

	USART_Cmd(USART1, ENABLE);
	GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);

}

int main(void)
{
	init_hw();

	for(;;)//int i=0; i< 10000; i++)
	{
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
		_send_string(USART1, "Line 1\n");
		_mydelay(1000000);

		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
		_send_string(USART1, "Line 2\n");
		_mydelay(1000000);
	}
	while(1);
}

