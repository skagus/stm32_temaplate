#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif 

	void SPI1_Init();
	uint8 SPI1_Tx(uint8 nData);
	void SPI1_DmaTx(uint8* pRx, uint8* pTx, uint16_t nLen);

#ifdef __cplusplus
}
#endif

