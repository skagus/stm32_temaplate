#include <stm32f10x_conf.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_rcc.h>

int i = 0;
int off = 5;
#define UNUSED(x)		(void)(x)

void assert_failed(uint8_t* file, uint32_t line)
{
	UNUSED(file);
	UNUSED(line);
}

void inc(void)
{
	i += off;
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

int main(void) __attribute__((section(".txt2")));

int main(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = GPIO_Pin_13;
	stGpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC, &stGpioInit);
	GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);

	USART_InitTypeDef stInitUart;
	stInitUart.USART_BaudRate = 115200;
	stInitUart.USART_WordLength = USART_WordLength_8b;
	stInitUart.USART_StopBits = USART_StopBits_1;
	stInitUart.USART_Parity = USART_Parity_No;
	stInitUart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	stInitUart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &stInitUart);

	for(;;)//int i=0; i< 10000; i++)
	{
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
//		_send_string(USART1, "Line 1\n");
		_mydelay(10000);

		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
//		_send_string(USART1, "Line 2\n");
		_mydelay(10000);
	}
	while(1);
}

