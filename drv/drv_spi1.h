#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif 

	void SPI1_Init();
	uint8 SPI1_IO(uint8 nData);
	void SPI1_DmaIO(uint8* pRx, uint8* pTx, uint16_t nLen);
	void SPI1_DmaOut(uint8* pTx, uint16_t nLen);
	void SPI1_DmaIn(uint8* pRx, uint16_t nLen);

#ifdef __cplusplus
}
#endif

