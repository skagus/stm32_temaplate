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

uint8* PrintBuf::PQ_GetAddPtr(uint32* pnMaxLen)
{
	uint32 nContLen = UART_TX_BUF_LEN + UART_TX_BUF_ADD - nNextAdd;
	*pnMaxLen = (nFree < nContLen) ? nFree : nContLen;
	return aBuf + nNextAdd;
}

void PrintBuf::PQ_UpdateAddPtr(uint32 nLen)
{
	if (nNextAdd + nLen > UART_TX_BUF_LEN)
	{
		uint32 nCpyLen = nNextAdd + nLen - UART_TX_BUF_LEN;
		memcpy(aBuf, aBuf + UART_TX_BUF_LEN, nCpyLen);
	}
	nValid += nLen;
	nNextAdd = (nNextAdd + nLen) % UART_TX_BUF_LEN;
	nFree -= nLen;
}

/**
* @return Valid data의 크기.
*/
uint16 PrintBuf::PQ_UpdateDelPtr()
{
	nNextDelete = (nNextDelete + nOnDelete) % UART_TX_BUF_LEN;
	nFree += nOnDelete;
	nValid -= nOnDelete;
	nOnDelete = 0;
	return nValid;
}

uint8* PrintBuf::PQ_GetDelPtr(uint32* pnSize)
{
	uint8* pNxtDma = NULL;

	if (nValid > 0)
	{
		if (nNextDelete + nValid < UART_TX_BUF_LEN) // normal case.
		{
			*pnSize = nValid;
		}
		else // roll over case.
		{
			*pnSize = UART_TX_BUF_LEN - nNextDelete;
		}
		pNxtDma = aBuf + nNextDelete;
	}
	else
	{
		*pnSize = 0;
	}
	nOnDelete = *pnSize;
	return pNxtDma;
}

