#pragma once

#define MS_PER_TICK		(10)
#define SCHED_MSEC(x)	((x)/MS_PER_TICK)
#define LONG_TIME		(SCHED_MSEC(3000))	// 3 sec.

typedef enum
{
	EVT_UART_RX,
	EVT_UART_TX,
	EVT_ECHO,
	NUM_EVT
} evt_id;

/**
* Run mode는 특정 상태에서 runnable task관리를 용이하도록 하기 위함.
*/
typedef enum
{
	MODE_NORMAL,	// default mode
	MODE_SLEEP,
	MODE_FAIL_SAFE,
	NUM_MODE,
} RunMode;

typedef enum
{
	TID_LED,
	TID_ECHO,
	NUM_TASK,
} TaskId;
