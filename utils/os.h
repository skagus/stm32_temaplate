#pragma once

#include "types.h"
#include "os_conf.h"

#define MAX_OS				(1)		///< Number of max OS (>= NUM CPU.
#define MAX_EVT				(32)			///< Maximum kinds of event.
#define MAX_TASK			(8)
#define HANG_TIME			(OS_SEC(5))		///< 
#define DEF_TASK_GRP		(FF08)			///< Task group for default group.
#define INV_TASK			(FF08)			///< Invalid task id.

typedef void(*Task)(void* param);

#if defined(__cplusplus)
extern "C"
{
#endif
	Cbf OS_Init(void);
	uint8 OS_CreateTask(Task pfEntry, void* pStkTop, void* nParam, const char* szName);
	void OS_Start(void);
	uint32 OS_GetTick(void);
	uint32 OS_Wait(uint32 bmEvt, uint32 nTO);
	uint32 OS_SyncEvt(uint32 bmEvt);
	void OS_AsyncEvt(uint32 nEvtID);
	void OS_Stop(uint32 bmTask);
	void OS_Lock(uint32 bmLock);
	void OS_Unlock(uint32 bmLock);

#if defined(__cplusplus)
}
#endif

#define OS_Idle(nTimeout)	OS_Wait(0, nTimeout)
#define OS_Yield()			OS_Wait(0, 0)	
