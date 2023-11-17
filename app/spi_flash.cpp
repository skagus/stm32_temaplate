
#include <stm32f10x_gpio.h>
#include "config.h"
#include "macro.h"
#include "print.h"
#include "os.h"
#include "drv_spi1.h"
#include "cli.h"
#include "spi_flash.h"

#define CMD_READ			0x03
#define CMD_ERASE			0x20
#define CMD_PGM				0x02
#define CMD_WREN			0x06
#define CMD_READWRSR		0x01
#define CMD_READRDSR		0x05
#define CMD_READ_PROT		0x3C
#define CMD_UNPROTECT		0x39
#define CMD_PROTECT			0x36
#define CMD_READID			0x9F

#define SIZE_BUF			(16)

#define DBG(...)			UT_Printf(__VA_ARGS__)

#define SF_ENABLE()			{OS_Lock(BIT(LOCK_SPI1));GPIO_ResetBits(FLASH_PORT, FLASH_CS);}
#define SF_DISABLE()		{GPIO_SetBits(FLASH_PORT, FLASH_CS);OS_Unlock(BIT(LOCK_SPI1));}


void sf_Read(uint8* pRxBuf, uint16 nBytes)
{
	if (nBytes >= 8)
	{
		SPI1_DmaIn(pRxBuf, nBytes);
	}
	else
	{
		for (int i = 0; i < nBytes; i++)
		{
			pRxBuf[i] = SPI1_IO(0x00);
		}
	}
}

void sf_Write(uint8* pTxBuf, uint16 nBytes)
{
	if (nBytes >= 8)
	{
		SPI1_DmaOut(pTxBuf, nBytes);
	}
	else
	{
		for (int i = 0; i < nBytes; i++)
		{
			SPI1_IO(pTxBuf[i]);
		}
	}
}

void sf_ReadWrite(uint8* pRxBuf, uint8* pTxBuf, uint16 nBytes)
{
	SF_ENABLE();
	if (nBytes >= 8)
	{
		SPI1_DmaIO(pRxBuf, pTxBuf, nBytes);
	}
	else
	{
		for (int i = 0; i < nBytes; i++)
		{
			pRxBuf[i] = SPI1_IO(pTxBuf[i]);
		}
	}
	SF_DISABLE();
}


inline void _IssueCmd(uint8* aCmds, uint8 nLen)
{
	SF_ENABLE();
	sf_Write(aCmds, nLen);
	SF_DISABLE();
}

inline void _WriteEnable(void)
{
	uint8 nCmd = CMD_WREN;
	_IssueCmd(&nCmd, 1);
}

inline uint8 _WaitBusy(void)
{
	uint8 nCmd = CMD_READRDSR;
	uint8 nResp = 0;
	SF_ENABLE();
	while (1)
	{
		sf_Write(&nCmd, 1);
		sf_Read(&nResp, 1);
		if (0 == (nResp & 0x01)) // check busy.
		{
			break;
		}
	}
	SF_DISABLE();
	return nResp;
}


uint32 _ReadId(void)
{
	uint8 anCmd[4] = { CMD_READID,0,0,0 };
	SF_ENABLE();
	sf_Write(anCmd, 1);
	sf_Read(anCmd, 4);
	SF_DISABLE();
	return *(uint32*)anCmd;
}

void _WriteStatus(void)
{
	uint8 anCmd[2];
	anCmd[0] = CMD_READWRSR;
	anCmd[1] = 0;
	_IssueCmd(anCmd, 2);
}

void _UnProtect(uint32 nAddr)
{
	uint8 anCmd[4];
	anCmd[0] = CMD_UNPROTECT;
	anCmd[1] = (nAddr >> 16) & 0xFF;
	anCmd[2] = (nAddr >> 8) & 0xFF;
	anCmd[3] = nAddr & 0xFF;
	_WriteEnable();
	_IssueCmd(anCmd, 4);
}

void _Protect(uint32 nAddr)
{
	uint8 anCmd[4];
	anCmd[0] = CMD_PROTECT;
	anCmd[1] = (nAddr >> 16) & 0xFF;
	anCmd[2] = (nAddr >> 8) & 0xFF;
	anCmd[3] = nAddr & 0xFF;
	_WriteEnable();
	_IssueCmd(anCmd, 4);
}

uint8 _ReadProt(uint32 nAddr)
{
	uint8 nResp;
	uint8 anCmd[4];
	anCmd[0] = CMD_READ_PROT;
	anCmd[1] = (nAddr >> 16) & 0xFF;
	anCmd[2] = (nAddr >> 8) & 0xFF;
	anCmd[3] = nAddr & 0xFF;

	SF_ENABLE();
	sf_Write(anCmd, 4);
	sf_Read(&nResp, 1);
	SF_DISABLE();

	return nResp;
}


uint8 _Erase(uint32 nSAddr)
{
	DBG("E:%X ", nSAddr);

	uint32 nAddr = ALIGN_UP(nSAddr, SECT_SIZE);
	uint8 anCmd[4] = { CMD_ERASE, };
	anCmd[1] = (nAddr >> 16) & 0xFF;
	anCmd[2] = (nAddr >> 8) & 0xFF;
	anCmd[3] = nAddr & 0xFF;

	_UnProtect(nAddr);
	_WriteEnable();
	_IssueCmd(anCmd, 4);
	uint8 nRet = _WaitBusy();
	DBG("->%X\n", nRet);
	return nRet;
}


uint8 _Write(uint8* pBuf, uint32 nAddr, uint32 nByte)
{
	DBG("W:%X,%d ", nAddr, nByte);

	uint8 anCmd[4];
	anCmd[0] = CMD_PGM;
	anCmd[1] = (nAddr >> 16) & 0xFF;
	anCmd[2] = (nAddr >> 8) & 0xFF;
	anCmd[3] = nAddr & 0xFF;

	_UnProtect(nAddr);
	_WriteEnable();
	SF_ENABLE();
	sf_Write(anCmd, 4);
	sf_Read(pBuf, nByte);
	SF_DISABLE();

	uint8 nRet = _WaitBusy();
	DBG("->%X\n", nRet);
	return nRet;
}

void _Read(uint8* pBuf, uint32 nAddr, uint32 nByte)
{
	DBG("R:%X,%d\n", nAddr, nByte);

	uint8 anCmd[4];
	anCmd[0] = CMD_READ;
	anCmd[1] = (nAddr >> 16) & 0xFF;
	anCmd[2] = (nAddr >> 8) & 0xFF;
	anCmd[3] = nAddr & 0xFF;

	SF_ENABLE();
	sf_Write(anCmd, 4);
	sf_Read(pBuf, nByte);
	SF_DISABLE();
}

void FLASH_Read(uint8* pBuf, uint32 nSAddr, uint32 nInLen)
{
	uint32 nAddr = nSAddr;
	uint32 nRest = nInLen;
	uint32 nLen = PAGE_SIZE - (nSAddr % PAGE_SIZE); // to fit align.
	if (nLen > nRest) nLen = nRest;
	while (nRest > 0)
	{
		_Read(pBuf, nAddr, nLen);
		pBuf += nLen;
		nAddr += nLen;
		nRest -= nLen;
		nLen = MIN(nRest, PAGE_SIZE);
	}
}

uint8 FLASH_Write(uint8* pBuf, uint32 nSAddr, uint32 nInLen)
{
	uint8 nRet = 0xFF;
	uint32 nAddr = nSAddr;
	uint32 nRest = nInLen;
	uint32 nLen = MIN(PAGE_SIZE - (nSAddr % PAGE_SIZE), nRest); // to fit align.

	while (nRest > 0)
	{
		if (0 == (nAddr % SECT_SIZE))
		{
			nRet = _Erase(nAddr);
		}
		OS_Idle(OS_MSEC(100));
		nRet = _Write(pBuf, nAddr, nLen);
		pBuf += nLen;
		nAddr += nLen;
		nRest -= nLen;
		nLen = MIN(nRest, PAGE_SIZE);
	}
	return nRet;
}

uint8 FLASH_Erase(uint32 nSAddr, uint32 nLen)
{
	uint8 nRet = 0xFF;
	uint32 nAddr = ALIGN_UP(nSAddr, SECT_SIZE);
	uint32 nEAddr = ALIGN_DN(nSAddr + nLen, SECT_SIZE);
	while (nAddr < nEAddr)
	{
		nRet = _Erase(nAddr);
		nAddr += SECT_SIZE;
	}
	return nRet;
}

#if 0
typedef struct _YmParam
{
	uint32 nAddr;
	uint32 nByte;
} YmParam;
YmParam gYParam;

bool _ProcWrite(uint8* pBuf, uint32* pnBytes, YMState eStep, void* pParam)
{
	bool bRet = true;
	YmParam* pInfo = (YmParam*)pParam;
	switch (eStep)
	{
	case YS_META:
	{
		pInfo->nByte = *pnBytes;
		break;
	}
	case YS_DATA:
	{
		uint32 nLen = MIN(pInfo->nByte, *pnBytes);
		FLASH_Write(pBuf, pInfo->nAddr, nLen);
		pInfo->nAddr += nLen;
		pInfo->nByte -= nLen;
		break;
	}
	default:
	{
		break;
	}
	}
	return bRet;
}

bool _ProcRead(uint8* pBuf, uint32* pnBytes, YMState eStep, void* pParam)
{
	bool bRet = true;
	YmParam* pInfo = (YmParam*)pParam;
	switch (eStep)
	{
	case YS_META:
	{
		sprintf((char*)pBuf, "0x%X_fr.bin", (int)pInfo->nAddr);
		*pnBytes = pInfo->nByte;
		break;
	}
	case YS_DATA:
	{
		uint32 nLen = MIN(pInfo->nByte, *pnBytes);
		FLASH_Read(pBuf, pInfo->nAddr, nLen);
		pInfo->nAddr += nLen;
		pInfo->nByte -= nLen;
		break;
	}
	default:
	{
		break;
	}
	}
	return bRet;
}
#endif

void _printUsage(void)
{
	UT_Printf("\ti - show flash ID\n");
	UT_Printf("\te <Addr> <Size> - Erase flash\n");
	UT_Printf("\tr <Addr> <Size> - Read flash and show\n");
	UT_Printf("\tf <Addr> <Size> [Pat=0xAA] - Fill flash with pattern\n");
	UT_Printf("\tW <Addr> - Write received data (user Ymodem)\n");
	UT_Printf("\tR <Addr> <Size> - Read flash and send it via Ymodem\n");
}

void sf_Cmd(uint8 argc, char* argv[])
{
	if (argc < 2)
	{
		_printUsage();
		return;
	}

	uint8 aBuf[PAGE_SIZE];

	// flash <cmd> <addr> <size> <opt>
	char nCmd = argv[1][0];
	uint32 nAddr = 0;
	uint32 nByte = 0;

	if (argc >= 3)
	{
		nAddr = CLI_GetInt(argv[2]);
	}
	if (argc >= 4)
	{
		nByte = CLI_GetInt(argv[3]);
	}

	if (nCmd == 'i')
	{
		uint32 nId = _ReadId();
		UT_Printf("FLASH ID: %X\n", nId);
	}
	else if (nCmd == 'r' && argc >= 4) // r 8
	{
		nAddr = ALIGN_DN(nAddr, PAGE_SIZE);
		UT_Printf("Flash Read: %X, %d\n", nAddr, nByte);
		while (nByte > 0)
		{
			uint32 nThis = nByte > PAGE_SIZE ? PAGE_SIZE : nByte;
			FLASH_Read(aBuf, nAddr, nThis);
			UT_Printf("Read: %X, %d\n", nAddr, nThis);
			// UT_DumpData(aBuf, nThis);
			nAddr += nThis;
			nByte -= nThis;
		}
	}
	else if (nCmd == 'f' && argc >= 4) // cmd w addr byte
	{
		uint8 nVal = (argc < 5) ? 0xAA : CLI_GetInt(argv[4]);

		memset(aBuf, nVal, PAGE_SIZE);
		UT_Printf("FLASH Write: %X, %d, %X\n", nAddr, nByte, nVal);
		while (nByte > 0)
		{
			uint32 nThis = nByte > PAGE_SIZE ? PAGE_SIZE : nByte;
			uint8 nRet = FLASH_Write(aBuf, nAddr, nThis);
			UT_Printf("Page Write: %X, %d --> %X\n", nAddr, nThis, nRet);
			nAddr += PAGE_SIZE;
			nByte -= PAGE_SIZE;
		}
	}
	else if (nCmd == 'e' && argc >= 4) //
	{
		uint8 nRet = FLASH_Erase(nAddr, nByte);
		UT_Printf("FLASH Erase: %X, %d --> %X\n", nAddr, nByte, nRet);
	}
#if 0
	else if (nCmd == 'W' && argc >= 3) // Write with Y modem.
	{
		gYParam.nAddr = nAddr;
		YReq stReq = { bReq:true, bRx : true, pfHandle : _ProcWrite, pParam : (void*)&gYParam };
		YM_Request(&stReq);
	}
	else if (nCmd == 'R' && argc >= 4) // Write with Y modem.
	{
		gYParam.nAddr = nAddr;
		gYParam.nByte = nByte;
		YReq stReq = { bReq:true, bRx : false, pfHandle : _ProcRead, pParam : (void*)&gYParam };
		YM_Request(&stReq);
	}
#endif
	else
	{
		_printUsage();
	}
}

void SF_Init()
{
	GPIO_InitTypeDef stGpioInit;
	GPIO_SetBits(FLASH_PORT, FLASH_CS);
	stGpioInit.GPIO_Pin = FLASH_CS;
	stGpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	stGpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(FLASH_PORT, &stGpioInit);

	// Init SPI if ... 
	CLI_Register((const char*)"sf", sf_Cmd);

}

