#include <stdint.h>
#include "console.h"
#include "led.h"

#define UNUSED(x)		(void)(x)

void assert_failed(uint8_t* file, uint32_t line)
{
	UNUSED(file);
	UNUSED(line);
}

void _mydelay(uint32_t nCycles)
{
	while(nCycles-- > 0)
	{
		__asm volatile("nop");
	}
}

int gnLoop = 100;
extern const char* gpVersion;
int main(void)
{
	CON_Init();
	LED_Init();

	CON_Printf("\nVER: %s\n", gpVersion);

	int nCnt = gnLoop;
	for(;;)//int i=0; i< 10000; i++)
	{
		LED_Set(1);
		CON_Printf("On:  %4d\n", nCnt);
		_mydelay(1000000);

		LED_Set(0);
		CON_Printf("Off: %4d\n", nCnt);
		_mydelay(1000000);
		nCnt++;
	}
	while(1);
}

