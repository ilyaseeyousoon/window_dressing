
// Header: SFCL_Driver_3_0 Firmware v1.0
// File Name: Init.h  (header for Init.c)
// Abstract: Separate INIT functions for all blocks which is needed 
// Author: D. Shelestov
// Date: 07.05.2015


#ifndef INIT_H
#define INIT_H

#include <stdint.h>
#include "main.h"
#include "stdio.h"
#include "stdbool.h"
#include "stm32f207xx.h"
#include "stm32f2xx.h"
#include "stm32f2xx_hal.h"
#include "stm32f2xx_hal_conf.h"
#include "stm32f2xx_hal_dma.h"
#include "stm32f2xx_hal_gpio.h"
#include "stm32f2xx_hal_gpio_ex.h"
#include "stm32f2xx_hal_dac.h"
#include "stm32f2xx_hal_adc.h"
#include "stm32f2xx_hal_adc_ex.h"
#include "stm32f2xx_hal_spi.h"
#include "stm32f2xx_hal_uart.h"
#include "stm32f2xx_hal_cortex.h"



/* GPIO Pin identifier */
typedef struct _GPIO_PIN {
  GPIO_TypeDef *port;
  uint16_t      pin;
} GPIO_PIN;

/* LED GPIO Pins */
static const GPIO_PIN LED_PIN[] = {
  {GPIOD, GPIO_PIN_2}, {GPIOD, GPIO_PIN_3} 
};
/* Button GPIO Pins */
static const GPIO_PIN But_PIN[] = {
  {GPIOD, GPIO_PIN_4}, {GPIOD, GPIO_PIN_5} 
};
/* Button GPIO Pins */
static const GPIO_PIN ALL_PIN[] = {
  {GPIOD, GPIO_PIN_0}, {GPIOD, GPIO_PIN_1}, {GPIOB, GPIO_PIN_8}, {GPIOB, GPIO_PIN_9}
};

#define LED_COUNT (sizeof(LED_PIN)/sizeof(GPIO_PIN))


int32_t ALL_PINS_Initialize (void); 
int32_t LED_Uninitialize (void);
int32_t LED_On (uint32_t num);
int32_t LED_Off (uint32_t num);
int32_t TEC_On (void);
int32_t TEC_Off (void);
int32_t TEC_Set (uint16_t setpoint);
int32_t LD_On (uint16_t c_setpoint, uint16_t p_setpoint);
int32_t LD_Off (void);
void HAL_UART_MspInit(UART_HandleTypeDef *huart);
//int32_t UART3_Initialize (void);
int32_t my_SPI3_Initialize (void);
int32_t DAC_Initialize(void);
int32_t ADC_Initialize(void);
int32_t DMA_Initialize(void);
int32_t IRQ_Initialize(void);
int32_t FLASH_Initialize (void);


#endif
