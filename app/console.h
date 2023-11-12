#pragma once

#include "types.h"
#include "macro.h"
#include "print_queue.h"



void CON_Init();
int CON_Printf(const char* szFmt, ...);
int CON_Puts(const char* szStr);

