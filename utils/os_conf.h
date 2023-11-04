#pragma once

#define MS_PER_TICK				(10)	///< sim tick period per OS tick.
#define OS_MSEC(msec)			((msec) / MS_PER_TICK)	///< tick per msec.
#define OS_SEC(sec)				((sec) * OS_MSEC(1000UL))	///< tick per sec.

#define LONG_TIME				(OS_SEC(3))	// 3 sec.

enum FTLEvt
{
	EVT_LOCK,	///< For OS.

	EVT_UART_RX,
	EVT_UART_TX,
	NUM_OS_EVT
};

enum FTL_Lock
{
	LOCK_UART,			///< UART 사용을 위한 Locking.
	NUM_OS_LOCK,
};
