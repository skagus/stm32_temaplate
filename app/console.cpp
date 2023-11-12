#include <stdarg.h>
#include <stdio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_dma.h>
#include "debug_cm3.h"
#include "config.h"
#include "misc.h"
#include "types.h"
#include "macro.h"
#include "os.h"
#include "led_matrix.h"
#include "console.h"
#include "drv_uart.h"

/**
 DMA 사용 전략.
	1. RX
		- Buffer로 일단 RX 걸어 놓음 --> Half/Full ISR
		- Idle이면 ISR call --> RX Event trigger.
	2. TX
		- TX도 buffering이 필요함.
		- buffering은 printf에서 마지막으로 실행.
		- Buffer를 circular로 사용하도록.
		- circular시에 문제점은 sprintf를 하기가 곤란하다는 점이 있다.
		- 이를 좀 더 쉽게 풀기 위해서, 꼬리달린 circular 적용한다.
		- +-----------------#---------@
		- + ~ # 까지만 circular 로 운용하는데, sprintf 에서는 buffer 끝은 @ 까지 허용함.
		- 하지만, 새로운 print 시점에서 current buffer위치가 #를 넘어가지는 않도록.
		- print결과 #가 넘어간 경우에는, + 쪽으로 copy작업하여 circular 유지.
		- 즉, circular로 사용하되, sprintf를 간단히 하기 위해서 꼬리를 허용하는 개념임.
*/


//////////////////////////////////////////////////

int CON_Puts(const char* szStr)
{
	uint32 nBufLen;
	uint8* pBuf = gstTxBuf.PQ_GetAddPtr(&nBufLen);
	uint32 nStrLen = strlen(szStr);
	uint32 nCpyLen = MIN(nStrLen, nBufLen);
	memcpy(pBuf, szStr, nCpyLen);

	__disable_irq();
	gstTxBuf.PQ_UpdateAddPtr(nCpyLen);
	UART_DmaTx();
	__enable_irq();

	return nCpyLen;
}

int CON_Printf(const char* szFmt, ...)
{
	uint32 nBufLen;
	uint8* pBuf = UART_GetWrteBuf(&nBufLen);

	va_list stVA;
	int nLen;

	va_start(stVA, szFmt);
	nLen = vsnprintf((char*)pBuf, nBufLen, szFmt, stVA);
	va_end(stVA);

	UART_PushWriteBuf(nLen);

	return nLen;
}

void CON_Flush()
{
	while (UART_CountTxFree() < 128);
}

/**
Console Task.
*/
void con_Run(void* pParam)
{
	uint8 aBuf[128];
	while (1)
	{
		uint32 nBytes = UART_GetData(aBuf, 128);
		if (nBytes > 0)
		{
			LEDMat_SendCh(aBuf[0]);
			aBuf[nBytes] = 0;
			CON_Printf("%s", aBuf);
		}
		OS_Wait(BIT(EVT_UART_RX), OS_MSEC(5000));
	}
}

#define SIZE_STK	(128)
static uint32 _aStk[SIZE_STK + 1];

void CON_Init()
{
	UART_Config();
	UART_DmaConfig();

	OS_CreateTask(con_Run, _aStk + SIZE_STK, NULL, "Con");
	//	gstTxBuf.Init();
}
