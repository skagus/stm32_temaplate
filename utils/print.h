#pragma once

#include "types.h"

void UT_PutCh(uint8 nCh);
int UT_Puts(const char* szStr);
int UT_Printf(const char* szFmt, ...);
void UT_Flush();
void UT_InitPrint();
