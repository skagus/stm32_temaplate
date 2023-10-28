#include <string.h>
#include "print_queue.h"

#if 0
class Test
{
	private:
	Test() {};

	public:
	int Func() { return 0; };
};
#endif

uint8* PQ_GetAddPtr(PrintBuf* pTxBuf, uint32* pnMaxLen)
{
	uint32 nContLen = UART_TX_BUF_LEN + UART_TX_BUF_ADD - pTxBuf->nNextAdd;
	*pnMaxLen = (pTxBuf->nFree < nContLen) ? pTxBuf->nFree : nContLen;
	return pTxBuf->aBuf + pTxBuf->nNextAdd;
}

void PQ_UpdateAddPtr(PrintBuf* pTxBuf, uint32 nLen)
{
	if (pTxBuf->nNextAdd + nLen > UART_TX_BUF_LEN)
	{
		uint32 nCpyLen = pTxBuf->nNextAdd + nLen - UART_TX_BUF_LEN;
		memcpy(pTxBuf->aBuf, pTxBuf->aBuf + UART_TX_BUF_LEN, nCpyLen);
	}
	pTxBuf->nValid += nLen;
	pTxBuf->nNextAdd = (pTxBuf->nNextAdd + nLen) % UART_TX_BUF_LEN;
	pTxBuf->nFree -= nLen;
}

/**
* @return Valid data의 크기.
*/
uint16 PQ_UpdateDelPtr(PrintBuf* pTxBuf)
{
	pTxBuf->nNextDelete = (pTxBuf->nNextDelete + pTxBuf->nOnDelete) % UART_TX_BUF_LEN;
	pTxBuf->nFree += pTxBuf->nOnDelete;
	pTxBuf->nValid -= pTxBuf->nOnDelete;
	pTxBuf->nOnDelete = 0;
	return pTxBuf->nValid;
}

uint8* PQ_GetDelPtr(PrintBuf* pTxBuf, uint32* pnSize)
{
	uint8* pNxtDma = NULL;

	if (pTxBuf->nValid > 0)
	{
		if (pTxBuf->nNextDelete + pTxBuf->nValid < UART_TX_BUF_LEN) // normal case.
		{
			*pnSize = pTxBuf->nValid;
		}
		else // roll over case.
		{
			*pnSize = UART_TX_BUF_LEN - pTxBuf->nNextDelete;
		}
		pNxtDma = pTxBuf->aBuf + pTxBuf->nNextDelete;
	}
	else
	{
		*pnSize = 0;
	}
	pTxBuf->nOnDelete = *pnSize;
	return pNxtDma;
}

