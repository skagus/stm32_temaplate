#pragma once

#include "types.h"

#define UART_TX_BUF_LEN			(512)
#define UART_TX_BUF_ADD			(128)

class PrintBuf
{
	private:

	uint16 nFree; ///< Free data.
	uint16 nValid; ///< Valid data의 양.
	uint16 nOnDelete; ///< Delete하고 있는 중인 data.
	uint16 nNextAdd; ///< 다음에 추가할 위치.
	uint16 nNextDelete; ///< 다음에 Delete할 위치.
	uint8 aBuf[UART_TX_BUF_LEN + UART_TX_BUF_ADD];

	public:
	PrintBuf() { nFree = UART_TX_BUF_LEN; }
	uint8* PQ_GetAddPtr(uint32* pnMaxLen);
	void PQ_UpdateAddPtr(uint32 nLen);
	uint8* PQ_GetDelPtr(uint32* pnSize);
	uint16 PQ_UpdateDelPtr();
};


