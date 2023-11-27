
#include <stm32f10x_gpio.h>
#include "config.h"
#include "macro.h"
#include "print.h"
#include "os.h"
#include "drv_spi1.h"
#include "cli.h"
#include "spi_flash.h"

#define CMD_READ			0x03	// [03:Ah:Am:Al:(Data 계속)]
#define CMD_FAST_READ		0x0B	// [03:Ah:Am:Al:Dummy:(Data 계속)]

#define CMD_ERASE_PAGE		0x20	// Sector(4KB) Erase [20:Ah:Am:Al]
#define CMD_ERASE_32K		0x52	// 32KB Block Erase [52:Ah:Am:Al]
#define CMD_ERASE_64K		0xD8	// 64KB Block Erase [D8:Ah:Am:Al]
#define CMD_ERASE_CHIP		0xC7	// Chip Erase [C7], 0x60 command도 가능.
#define CMD_PGM_PAGE		0x02	// Page Pgm [02:Ah:Am:Al:(Data 256B)]

#define CMD_EN_WR			0x06	// Write enable [06]
#define CMD_DIS_WR			0x04	// Write disable [04]
#define CMD_WR_SR			0x01	// Write status reg  [01:Status1:Status2]
#define CMD_RD_SR1			0x05	// Read status reg 1 [05:(Status1 계속)~]
#define CMD_RD_SR2			0x35	// Read status reg 2 [35:(Status2 계속)~] : Status2는 2 bit만 valid.

#define CMD_RD_UID			0x4B	// Read Unique ID [90:dummy(4B):(UID:8B)]
#define CMD_RD_JEDEC		0x9F	// Read JEDEC ID. [9F:(Manu ID):(Mem Type):(Capacity)]

/**
* Status 1:
	[7] SRP0 : Status Register Protect 0 (non-volatile)
	[6] SEC : Sector protect (non-volatile)
	[5] TB : Top/Bottom write protect (non-volatile)
	[4] BP2 : Block protection bit. (non-volatile)
	[3] BP1 : Block protection bit. (non-volatile)
	[2] BP0 : Block protection bit. (non-volatile)
	[1] WEL : Write enable latch.
	[0] BUSY : Erase/Program in progress.
* Status 2:
	[1] QE: Quad enable.
	[0] SRP1 : Status register protect 1
*/

#define SIZE_BUF			(16)

#define DBG(...)			UT_Printf(__VA_ARGS__)

#define SF_ENABLE()			{OS_Lock(BIT(LOCK_SPI1));GPIO_ResetBits(FLASH_PORT, FLASH_CS);}
#define SF_DISABLE()		{GPIO_SetBits(FLASH_PORT, FLASH_CS);OS_Unlock(BIT(LOCK_SPI1));}


void spi_Read(uint8* pRxBuf, uint16 nBytes)
{
	memset(pRxBuf, 0xCC, nBytes);
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

void spi_Write(uint8* pTxBuf, uint16 nBytes)
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

void spi_ReadWrite(uint8* pRxBuf, uint8* pTxBuf, uint16 nBytes)
{
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
}

/**
 * The WEL bit must be set prior to every
 * Page Program,
 * Sector/Block/Chip Erase,
 * Write Status Register
 *
 * Update command가 끝나면, Write disable로 자동 전환.
*/
inline void _WriteEnable(void)
{
	uint8 nCmd = CMD_EN_WR;
	spi_Write(&nCmd, 1);
}

inline uint8 _WaitBusy(void)
{
	uint8 nCmd = CMD_RD_SR1;
	uint8 nResp = 0;
	SF_ENABLE();
	spi_Write(&nCmd, 1);
	while (1)
	{
		spi_Read(&nResp, 1);
		if (0 == (nResp & 0x01)) // check busy.
		{
			break;
		}
	}
	SF_DISABLE();
	return nResp;
}

// Need write enable.
void _WriteStatus(uint8 nStatus1, uint8 nStatus2)
{
	uint8 anCmd[3];
	anCmd[0] = CMD_WR_SR;
	anCmd[1] = nStatus1;
	anCmd[2] = nStatus2;
	spi_Write(anCmd, 3);
}

void sf_General(uint8* aInData, uint16 nInByte, uint8* aOutData, uint16 nOutByte)
{
	SF_ENABLE();
	spi_Write(aInData, nInByte);
	if (nOutByte > 0)
	{
		spi_Read(aOutData, nOutByte);
	}
	SF_DISABLE();
}

uint32 sf_ReadId(void)
{
	uint8 anCmd[4] = { CMD_RD_JEDEC,0,0,0 };
	SF_ENABLE();
	spi_Write(anCmd, 1);
	spi_Read(anCmd, 4);
	SF_DISABLE();
	return *(uint32*)anCmd;
}

void sf_UpdateStatus(uint8* aStatus)
{
	uint8 nCmd = CMD_RD_SR1;
	uint8 nSt1 = 0;
	SF_ENABLE();
	spi_Write(&nCmd, 1);
	spi_Read(&nSt1, 1);
	SF_DISABLE();

	nCmd = CMD_RD_SR2;
	uint8 nSt2 = 0;
	uint8 nResp = 0;
	SF_ENABLE();
	spi_Write(&nCmd, 1);
	spi_Read(&nSt2, 1);
	SF_DISABLE();

	UT_Printf("Org Status: %X %X\n", nSt1, nSt2);
	if (nullptr != aStatus)
	{
		uint8 anCmd[3];
		anCmd[0] = CMD_WR_SR;
		anCmd[1] = aStatus[0];
		anCmd[2] = aStatus[1];

		SF_ENABLE();
		_WriteEnable();
		SF_DISABLE();

		SF_ENABLE();
		spi_Write(anCmd, 3);
		SF_DISABLE();
	}
}

/**
 * TB, BP2, BP1, BP0 모두 문제 없어야 erase됨.
*/
uint8 sf_Erase(uint32 nSAddr)
{
	DBG("E:%X ", nSAddr);

	uint32 nAddr = ALIGN_UP(nSAddr, SECT_SIZE);
	uint8 anCmd[4] = { CMD_ERASE_PAGE, };
	anCmd[1] = (nAddr >> 16) & 0xFF;
	anCmd[2] = (nAddr >> 8) & 0xFF;
	anCmd[3] = nAddr & 0xFF;

	SF_ENABLE();
	_WriteEnable();
	SF_DISABLE(); // CS high needed just after comman seq.

	SF_ENABLE();
	spi_Write(anCmd, 4);
	SF_DISABLE(); // CS high needed just after comman seq.

	uint8 nRet = _WaitBusy();
	DBG("->%X\n", nRet);
	return nRet;
}


uint8 sf_Write(uint8* pBuf, uint32 nAddr, uint32 nByte)
{
	DBG("W:%X,%d ", nAddr, nByte);

	uint8 anCmd[4];
	anCmd[0] = CMD_PGM_PAGE;
	anCmd[1] = (nAddr >> 16) & 0xFF;
	anCmd[2] = (nAddr >> 8) & 0xFF;
	anCmd[3] = nAddr & 0xFF;

	SF_ENABLE();
	_WriteEnable();
	SF_DISABLE();

	SF_ENABLE();
	spi_Write(anCmd, 4);
	spi_Write(pBuf, nByte);
	SF_DISABLE();

	uint8 nRet = _WaitBusy();
	DBG("->%X\n", nRet);
	return nRet;
}

void sf_Read(uint8* pBuf, uint32 nAddr, uint32 nByte)
{
	DBG("R:%X,%d\n", nAddr, nByte);

	uint8 anCmd[4];
	anCmd[0] = CMD_READ;
	anCmd[1] = (nAddr >> 16) & 0xFF;
	anCmd[2] = (nAddr >> 8) & 0xFF;
	anCmd[3] = nAddr & 0xFF;

	SF_ENABLE();
	spi_Write(anCmd, 4);
	spi_Read(pBuf, nByte);
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
		sf_Read(pBuf, nAddr, nLen);
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
			nRet = sf_Erase(nAddr);
		}
		OS_Idle(OS_MSEC(100));
		nRet = sf_Write(pBuf, nAddr, nLen);
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
		nRet = sf_Erase(nAddr);
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
	UT_Printf("\tx <in size> <in data> <out size> - Do some command.\n");
	UT_Printf("\ts <Status 1> <Status 2> - Write status\n");
	UT_Printf("\te <Addr> <Size> - Erase flash\n");
	UT_Printf("\tr <Addr> <Size> - Read flash and show\n");
	UT_Printf("\tf <Addr> <Size> [Pat=0xAA] - Fill flash with pattern\n");
	//	UT_Printf("\tW <Addr> - Write received data (user Ymodem)\n");
	//	UT_Printf("\tR <Addr> <Size> - Read flash and send it via Ymodem\n");
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

	if (nCmd == 'i')
	{
		uint32 nId = sf_ReadId();
		UT_Printf("FLASH ID: %X\n", nId);
	}
	else if (nCmd == 'x' && argc == 5)
	{
		uint8 nInByte = CLI_GetInt(argv[2]);
		uint32 nInData = CLI_GetInt(argv[3]);
		uint8 nOutByte = CLI_GetInt(argv[4]);
		uint8 aOutData[128];
		sf_General((uint8*)&nInData, nInByte, aOutData, nOutByte);
		UT_DumpData(aOutData, nOutByte, 1);
	}
	else if (nCmd == 's')
	{
		if (3 == argc) // status 1,2
		{
			uint8 aNew[2];
			aNew[0] = (uint8)CLI_GetInt(argv[1]);
			aNew[1] = (uint8)CLI_GetInt(argv[2]);
			sf_UpdateStatus(aNew);
		}
		else
		{
			sf_UpdateStatus(nullptr);
		}
	}
	else if (nCmd == 'r' && argc >= 4) // r 8
	{
		nAddr = CLI_GetInt(argv[2]);
		nByte = CLI_GetInt(argv[3]);
		nAddr = ALIGN_DN(nAddr, PAGE_SIZE);
		UT_Printf("Flash Read: %X, %d\n", nAddr, nByte);
		while (nByte > 0)
		{
			uint32 nThis = nByte > PAGE_SIZE ? PAGE_SIZE : nByte;
			FLASH_Read(aBuf, nAddr, nThis);
			UT_Printf("Read: %X, %d\n", nAddr, nThis);
			UT_DumpData(aBuf, nThis, 4);
			nAddr += nThis;
			nByte -= nThis;
		}
	}
	else if (nCmd == 'f' && argc >= 4) // cmd w addr byte
	{
		nAddr = CLI_GetInt(argv[2]);
		nByte = CLI_GetInt(argv[3]);
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
		nAddr = CLI_GetInt(argv[2]);
		nByte = CLI_GetInt(argv[3]);
		uint8 nRet = FLASH_Erase(nAddr, nByte);
		UT_Printf("FLASH Erase: %X, %d --> %X\n", nAddr, nByte, nRet);
	}
#if 0
	else if (nCmd == 'W' && argc >= 3) // Write with Y modem.
	{
		nAddr = CLI_GetInt(argv[2]);
		gYParam.nAddr = nAddr;
		YReq stReq = { bReq:true, bRx : true, pfHandle : _ProcWrite, pParam : (void*)&gYParam };
		YM_Request(&stReq);
	}
	else if (nCmd == 'R' && argc >= 4) // Write with Y modem.
	{
		nAddr = CLI_GetInt(argv[2]);
		nByte = CLI_GetInt(argv[3]);
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

