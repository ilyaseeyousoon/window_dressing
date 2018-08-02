/*----------------------------------------------------------------------------
 * Name:    Serial.h
 * Purpose: Low level serial definitions
 * Note(s):
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

#include <stdint.h>
#include "Main.h"
#include "stm32f207xx.h"                  // Device header
#include "stm32f2xx_hal.h"
#include "stm32f2xx_hal_conf.h"


#ifndef SERIAL_H
#define SERIAL_H

extern uint8_t  SER_GetChar   (void);
extern uint8_t  SER_PutChar   (uint8_t c);

#endif
