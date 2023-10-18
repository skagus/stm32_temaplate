#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <stdarg.h>
#include <stdio.h>
#include <misc.h>
int gnCntUartInt;

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		uint16_t nRxData = USART_ReceiveData(USART1);
		USART_SendData(USART1, (char)nRxData);
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
	gnCntUartInt++;
}

void CON_Init()
{
	//// Enable GPIO A9, A10 for UART1.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef stGpioInit;
	stGpioInit.GPIO_Pin = GPIO_Pin_9;
	stGpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
	stGpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &stGpioInit);

	stGpioInit.GPIO_Pin = GPIO_Pin_10;
	stGpioInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &stGpioInit);

	//// Enable UART1.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	USART_InitTypeDef stInitUart;
	USART_StructInit(&stInitUart);
	stInitUart.USART_BaudRate = 115200;
	USART_Init(USART1, &stInitUart);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_InitTypeDef stCfgNVIC;
	stCfgNVIC.NVIC_IRQChannel = USART1_IRQn;
	stCfgNVIC.NVIC_IRQChannelSubPriority = 8;
	stCfgNVIC.NVIC_IRQChannelPreemptionPriority = 8;
	stCfgNVIC.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&stCfgNVIC);

	USART_Cmd(USART1, ENABLE);
}

void _send_char(char nCh)
{
	while(0 == (USART1->SR & (1<<7)));
	USART_SendData(USART1, nCh);
}

int CON_Puts(const char* szStr)
{
	while(*szStr)
	{
		_send_char(*szStr);
		szStr++;
	}
	return 0;
}

#define MAX_BUF_SIZE		(128)
char gaConBuf[MAX_BUF_SIZE];
int CON_Printf(const char* szFmt, ...)
{
	va_list ap;
	int len;

	va_start(ap, szFmt);
	len = vsprintf(gaConBuf, szFmt, ap);
	va_end(ap);

	CON_Puts(gaConBuf);
	return len;
}

