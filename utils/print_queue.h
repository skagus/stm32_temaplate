
#include "types.h"

#define UART_TX_BUF_LEN			(512)
#define UART_TX_BUF_ADD			(64)

typedef struct _PrintBuf
{
	uint16 nFree; ///< Free data.
	uint16 nValid; ///< Valid data의 양.
	uint16 nOnDelete; ///< Delete하고 있는 중인 data.
	uint16 nNextAdd; ///< 다음에 추가할 위치.
	uint16 nNextDelete; ///< 다음에 Delete할 위치.
	uint8 aBuf[UART_TX_BUF_LEN + UART_TX_BUF_ADD];
} PrintBuf;

uint8* PQ_GetAddPtr(PrintBuf* pTxBuf, uint32* pnMaxLen);
void PQ_UpdateAddPtr(PrintBuf* pTxBuf, uint32 nLen);
uint8* PQ_GetDelPtr(PrintBuf* pTxBuf, uint32* pnSize);
uint16 PQ_UpdateDelPtr(PrintBuf* pTxBuf);

