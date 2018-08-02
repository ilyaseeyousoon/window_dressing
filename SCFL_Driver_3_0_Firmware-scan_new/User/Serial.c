/*----------------------------------------------------------------------------
 * Name:    Serial.c
 * Purpose: Low Level Serial Routines
 * Note(s): possible defines select the used communication interface:
 *                        - USART2 interface  (default)
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2014 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include "Serial.h"

extern UART_HandleTypeDef *huart3;

/*----------------------------------------------------------------------------
  Write character to Serial Port
 *----------------------------------------------------------------------------*/
uint8_t SER_PutChar (uint8_t ch) {
	
	#ifdef _UART_CONSOLE_FULL
		HAL_UART_Transmit(huart3, &ch, 1, 100);	
	#endif
  return (ch);
	
}

/*----------------------------------------------------------------------------
  Read character from Serial Port
 *----------------------------------------------------------------------------*/
uint8_t SER_GetChar (void) {

 	uint8_t ser_temp;
	
	return (HAL_UART_Receive(huart3, &ser_temp, 1, 100));	

}
