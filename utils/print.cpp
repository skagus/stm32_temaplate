#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "macro.h"
#include "os.h"
#include "drv_uart.h"
#include "print.h"


void UT_PutCh(uint8 nCh)
{
	uint32 nBufLen;
	uint8* pBuf;

	while (true)
	{
		pBuf = UART_GetWriteBuf(&nBufLen);
		if (nullptr == pBuf)
		{
			UT_Flush();
			continue;
		}
		break;
	}

	*pBuf = nCh;
	UART_PushWriteBuf(1);
}


int UT_Puts(const char* szStr)
{
	uint32 nBufLen;
	uint8* pBuf = UART_GetWriteBuf(&nBufLen);

	uint32 nStrLen = strlen(szStr);
	uint32 nCpyLen = MIN(nStrLen, nBufLen);

	memcpy(pBuf, szStr, nCpyLen);

	UART_PushWriteBuf(nCpyLen);
	return nCpyLen;
}


int UT_Printf(const char* szFmt, ...)
{
	uint32 nBufLen;
	uint8* pBuf = UART_GetWriteBuf(&nBufLen);

	int nUsedLen;
	va_list stVA;
	va_start(stVA, szFmt);
	nUsedLen = vsnprintf((char*)pBuf, nBufLen, szFmt, stVA);
	va_end(stVA);

	UART_PushWriteBuf(nUsedLen);
	return nUsedLen;
}

void UT_Flush()
{
	while (UART_CountTxFree() < 128)
	{
		OS_Wait(BIT(EVT_UART_TX), OS_MSEC(10));
	}
}

void UT_InitPrint()
{
	UART_Config();
	UART_DmaConfig();
}
