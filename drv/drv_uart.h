#pragma once

#include "print_queue.h"

#define UART_RX_BUF_LEN			(128)

extern PrintBuf gstTxBuf;

void UART_DmaConfig();
void UART_Config();
void UART_DmaTx();

uint8* UART_GetWrteBuf(uint32* pLen);
void UART_PushWriteBuf(uint32 nLen);

uint32 UART_GetData(uint8* pBuf, uint32 nBufLen);
uint32 UART_CountTxFree();		///< 남아 있는 write buffer의 크기.
