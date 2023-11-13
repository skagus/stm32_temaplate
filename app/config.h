#pragma once

/******************
 * Global configuration.
 *
 * NEVER include other files!
 * Only self defined macros.
*/

#define CON_UART				USART1
#define CON_PORT				GPIOA
#define CON_TX					GPIO_Pin_9
#define CON_RX					GPIO_Pin_10
#define CON_RX_DMA_CH			DMA1_Channel5
#define CON_TX_DMA_CH			DMA1_Channel4


#define SPI1_PORT			 	GPIOA
#define SPI1_CLOCK				GPIO_Pin_5    // Clock
#define SPI1_MISO				GPIO_Pin_6    // Master Input to Slave Output
#define SPI1_MOSI				GPIO_Pin_7
#define SPI1_RX_DMA_CH			DMA1_Channel2
#define SPI1_TX_DMA_CH			DMA1_Channel3

#define LED_MAT_PORT			GPIOA
#define LED_MAT_CS				GPIO_Pin_4    // NSS PIN
