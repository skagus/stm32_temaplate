#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif 

	void SPI_Init4led();
	uint16 SPI_Tx(uint16 nData);

#ifdef __cplusplus
}
#endif

