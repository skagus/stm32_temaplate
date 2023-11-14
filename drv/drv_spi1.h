#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif 

	void SPI1_Init();
	uint16 SPI1_Tx(uint16 nData);
	void SPI1_DmaTx(uint8* pRx, uint8* pTx, uint16_t nLen);

#ifdef __cplusplus
}
#endif

