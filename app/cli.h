#pragma once

#include "types.h"
#include "macro.h"
#include "print_queue.h"



void CLI_Init();
int CLI_Printf(const char* szFmt, ...);
int CLI_Puts(const char* szStr);

