#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif 

	void SPI1_Init();
	uint16 SPI1_Tx(uint16 nData);

#ifdef __cplusplus
}
#endif

