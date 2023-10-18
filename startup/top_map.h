#pragma once

/**
 * This file should parsable by linker.
*/

#define KILO(x)			((x)*1024)

#define FLASH_BASE		(0x08000000)
#define SRAM_BASE		(0x20000000)

#define FLASH_SIZE		KILO(64)
#define SRAM_SIZE		KILO(8)


