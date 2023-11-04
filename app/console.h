#pragma once

#include "types.h"
#include "macro.h"
#include "print_queue.h"

#define UART_RX_BUF_LEN			(128)

extern PrintBuf gstTxBuf;
extern uint8 gaRxBuf[UART_RX_BUF_LEN];

void UART_DmaConfig();
void UART_Config();
void UART_DmaTx();

void CON_Init();
int CON_Printf(const char* szFmt, ...);
int CON_Puts(const char* szStr);

