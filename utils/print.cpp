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

void UT_DumpData(uint8* aInData, uint32 nByte, uint8 nWidth)
{
	switch (nWidth)
	{
	case 1:
	{
		uint8* aData = aInData;
		for (uint32 nIdx = 0; nIdx < nByte; nIdx++)
		{
			UT_Flush();
			if (0 == (nIdx % 32)) UT_Printf("\n");
			UT_Printf("%02X ", aData[nIdx]);
		}
		UT_Printf("\n");
		break;
	}
	case 2:
	{
		uint16* aData = (uint16*)aInData;
		for (uint32 nIdx = 0; nIdx < nByte / 2; nIdx++)
		{
			UT_Flush();
			if (0 == (nIdx % 16)) UT_Printf("\n");
			UT_Printf("%04X ", aData[nIdx]);
		}
		UT_Printf("\n");
		break;
	}
	case 4:
	{
		uint32* aData = (uint32*)aInData;
		for (uint32 nIdx = 0; nIdx < nByte / 4; nIdx++)
		{
			UT_Flush();
			if (0 == (nIdx % 8)) UT_Printf("\n");
			UT_Printf("%08X ", aData[nIdx]);
		}
		UT_Printf("\n");
		break;
	}
	}
}

void UT_InitPrint()
{
	UART_Config();
	UART_DmaConfig();
}
