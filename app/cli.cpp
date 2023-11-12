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
#include "cli.h"
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
#define LEN_LINE            (64)
#define MAX_CMD_COUNT       (8)
#define MAX_ARG_TOKEN       (8)		// Command line의 maximum argument 개수.


typedef struct _CmdInfo
{
	const char* szCmd;
	CmdHandler* pHandle;
} CmdInfo;

CmdInfo gaCmds[MAX_CMD_COUNT];
uint8 gnCmds;


void cli_PutCh(uint8 nCh)
{
	uint32 nBufLen;
	uint8* pBuf;

	while (true)
	{
		pBuf = UART_GetWriteBuf(&nBufLen);
		if (nullptr == pBuf)
		{
			CLI_Flush();
			continue;
		}
		break;
	}

	*pBuf = nCh;
	UART_PushWriteBuf(1);
}


int CLI_Puts(const char* szStr)
{
	uint32 nBufLen;
	uint8* pBuf = UART_GetWriteBuf(&nBufLen);

	uint32 nStrLen = strlen(szStr);
	uint32 nCpyLen = MIN(nStrLen, nBufLen);

	memcpy(pBuf, szStr, nCpyLen);

	UART_PushWriteBuf(nCpyLen);
	return nCpyLen;
}


int CLI_Printf(const char* szFmt, ...)
{
	uint32 nBufLen;
	uint8* pBuf = UART_GetWriteBuf(&nBufLen);

	va_list stVA;
	int nLen;

	va_start(stVA, szFmt);
	nLen = vsnprintf((char*)pBuf, nBufLen, szFmt, stVA);
	va_end(stVA);

	UART_PushWriteBuf(nLen);
	return nLen;
}

void CLI_Flush()
{
	while (UART_CountTxFree() < 128)
	{
		OS_Wait(BIT(EVT_UART_TX), OS_MSEC(10));
	}
}

/**
 * CMD runner.
*/
void cli_CmdHelp(uint8 argc, char* argv[])
{
	if (argc > 1)
	{
		uint32 nNum = CLI_GetInt(argv[1]);
		CLI_Printf("help with %08lx\r\n", nNum);
		char* aCh = (char*)&nNum;
		CLI_Printf("help with %02X %02X %02X %02X\r\n", aCh[0], aCh[1], aCh[2], aCh[3]);
	}
	else
	{
		for (uint8 nIdx = 0; nIdx < gnCmds; nIdx++)
		{
			CLI_Printf("%d: %s\r\n", nIdx, gaCmds[nIdx].szCmd);
		}
	}
}

void cli_RunCmd(char* szCmdLine)
{
	char* aTok[MAX_ARG_TOKEN];
	uint8 nCnt = 0;
	char* pTok = strtok(szCmdLine, " ");
	while ((NULL != pTok) && (nCnt < MAX_ARG_TOKEN))
	{
		aTok[nCnt++] = pTok;
		pTok = strtok(NULL, " ");
	}
	bool bExecute = false;
	if (nCnt > 0)
	{
		for (uint8 nIdx = 0; nIdx < gnCmds; nIdx++)
		{
			if (0 == strcmp(aTok[0], gaCmds[nIdx].szCmd))
			{
				gaCmds[nIdx].pHandle(nCnt, aTok);
				bExecute = true;
				break;
			}
		}
	}
	if (false == bExecute)
	{
		CLI_Printf("Unknown command: %s\r\n", szCmdLine);
	}
}

uint32 CLI_GetInt(char* szStr)
{
	uint32 nNum;
	char* pEnd;
	uint8 nLen = strlen(szStr);
	if ((szStr[0] == '0') && ((szStr[1] == 'b') || (szStr[1] == 'B'))) // Binary.
	{
		nNum = strtoul(szStr + 2, &pEnd, 2);
	}
	else
	{
		nNum = (uint32)strtoul(szStr, &pEnd, 0);
	}

	if ((pEnd - szStr) != nLen)
	{
		nNum = NOT_NUMBER;
	}
	return nNum;
}

void CLI_Register(const char* szCmd, CmdHandler* pfHandle)
{
	gaCmds[gnCmds].szCmd = szCmd;
	gaCmds[gnCmds].pHandle = pfHandle;
	gnCmds++;
}


/**
Console Task.
*/
void cli_Run(void* pParam)
{
	uint8 nLen = 0;
	char aLine[LEN_LINE];
	uint8 nCh;

	while (1)
	{
		while (UART_GetData(&nCh, 1) > 0)
		{
			if (' ' <= nCh && nCh <= '~') // ASCII. normal.
			{
				cli_PutCh(nCh);
				aLine[nLen] = nCh;
				nLen++;
			}
			else if (('\n' == nCh) || ('\r' == nCh))
			{
				if (nLen > 0)
				{
					aLine[nLen] = 0;
					CLI_Puts("\r\n");
					cli_RunCmd(aLine);
					nLen = 0;
				}
				CLI_Puts("\r\n$> ");
			}
			else if (0x7F == nCh) // backspace.
			{
				if (nLen > 0)
				{
					CLI_Puts("\b \b");
					nLen--;
				}
			}
			else
			{
				CLI_Printf("~ %X\r\n", nCh);
			}
		}
		OS_Wait(BIT(EVT_UART_RX), 0);
	}
}

#define SIZE_STK	(128)
static uint32 _aStk[SIZE_STK + 1];

void CLI_Init()
{
	UART_Config();
	UART_DmaConfig();

	CLI_Register("help", cli_CmdHelp);
	OS_CreateTask(cli_Run, _aStk + SIZE_STK, NULL, "Con");
	//	gstTxBuf.Init();
}
