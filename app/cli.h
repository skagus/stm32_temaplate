#pragma once

#include "types.h"
#include "macro.h"

#define NOT_NUMBER			(0x7FFFFFFF)

typedef void (CmdHandler)(uint8 argc, char* argv[]);

void CLI_Register(const char* szCmd, CmdHandler* pfHandle);
uint32 CLI_GetInt(char* szStr);

int CLI_Printf(const char* szFmt, ...);
void CLI_Flush();
int CLI_Puts(const char* szStr);

void CLI_Init();

