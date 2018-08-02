// Header: SFCL_Driver_3_0 Firmware v1.1
// File Name: IRQ.c
// Abstract: Interrupt Handlers
// Author: D. Shelestov
// Date: 07.06.2015

#include "IRQ.h"
extern uint8_t scan_request;
uint8_t tec_work=0;
uint16_t tec_value_old,tec_value_new=0;
uint16_t tec_value_old_ex_dac=0;
/*-----------------------------------------------------------------------------
  USART3_Handler
 *----------------------------------------------------------------------------*/

void SysTick_Handler(void)
{	int32_t tmp_tick;
	
	HAL_IncTick();
	tmp_tick = HAL_GetTick();
	
	// НАБЛЮДЕНИЕ ЗНАЧЕНИЙ ВНЕШНЕГО ФД (текущее и рабочая точка) ВО ВРЕМЯ СТАБИЛИЗАЦИИ
//	if ( ((tmp_tick % 1000) == 0 ) && (Stab_ON) ) {
//		printf("STAB_ON: ExtPD_avg = %d 0.5*ExtPD_outside = %7.2f Tld_set = %d \r\n",ExtPD_avg, ExtPD_outside*ExtPD_SETPOINT_DEPTH, Tld_set);
//	};
	#ifdef DAC_EX_INSTALLED
	if ( tec_work==1  ) {
		
		if(tec_value_new > tec_value_old) {
			tec_value_old = tec_value_old+1;
			SPI_DAC_Transmit(tec_value_old);
			//HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_2,DAC_ALIGN_12B_R, tec_value_old);
//				printf(" %010d:tec_value_old \r\n", tec_value_old);
//				printf(" %010d:tec_value_new \r\n", tec_value_new);
		}
		
		if(tec_value_new < tec_value_old) {
			tec_value_old = tec_value_old-1;
			SPI_DAC_Transmit(tec_value_old);
//			HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_2,DAC_ALIGN_12B_R, tec_value_old);
//			printf(" %010d:tec_value_old1 \r\n", tec_value_old);
//				printf(" %010d:tec_value_new1 \r\n", tec_value_new);
		}
			if(tec_value_new == tec_value_old) {
		tec_work=0;
//				{printf("tec_work_reset \r\n");}
	}
		
	}
	
	# else
	if ( tec_work==1  ) {
		
		if(tec_value_new > tec_value_old) {
			tec_value_old = tec_value_old+1;
			HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_2,DAC_ALIGN_12B_R, tec_value_old);
//				printf(" %010d:tec_value_old \r\n", tec_value_old);
//				printf(" %010d:tec_value_new \r\n", tec_value_new);
		}
		
		if(tec_value_new < tec_value_old) {
			tec_value_old = tec_value_old-1;
			HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_2,DAC_ALIGN_12B_R, tec_value_old);
//			printf(" %010d:tec_value_old1 \r\n", tec_value_old);
//				printf(" %010d:tec_value_new1 \r\n", tec_value_new);
		}
			if(tec_value_new == tec_value_old) {
		tec_work=0;
//				{printf("tec_work_reset \r\n");}
	}
		
	}
	#endif
	
	if ( ( (tmp_tick % 1000) == 0 ) && (scan_request==33 ) ) {
		
		scan();
	}
	if  ( ((tmp_tick % 100) == 0 ) && (tmp_tick > 3000 ) ) {
	 //ADC_Update();
		HAL_ADC_Start_IT(hadc1);
	};
	
	// Heartbeat
	if ( (tmp_tick % 250) == 0 )
		LED_On(0);
	if ( (tmp_tick % 500) == 0 )
		LED_Off(0);
}

void USART3_IRQHandler(void) {
	
	uint8_t temp = 0;
	uint32_t err = 0;
	if (__HAL_UART_GET_FLAG(huart3, UART_FLAG_RXNE) && __HAL_UART_GET_IT_SOURCE(huart3, UART_IT_RXNE)) {	
		__HAL_UART_DISABLE_IT(huart3, UART_IT_RXNE);
		if (huart3->RxXferCount < STR_LENGTH_MAX)	{ 
			err = HAL_UART_Receive(huart3, huart3->pRxBuffPtr, COMM_WIDTH, 100);
			if (err == HAL_TIMEOUT) {					
				huart3->ErrorCode = HAL_UART_ERROR_NONE;
				__HAL_UNLOCK(huart3);
				printf(" Command entry timeout, error: HAL_TIMEOUT %d \r\n", err); 
			};
			
			if (err == HAL_OK) {
				if (cmd.State != CMD_STATE_BUSY) {
					cmd.CmdCode = huart3->pRxBuffPtr[0];
					cmd.Byte2 = huart3->pRxBuffPtr[1]; 				
					cmd.Byte3 = huart3->pRxBuffPtr[2]; 
					cmd.Interface = CMD_INT_UART; 
					cmd.State = CMD_STATE_BUSY; 
					for (temp = 0; temp < COMM_WIDTH; temp++) {
						printf(" 0d%02d byte= 0x%02X \r\n",temp,huart3->pRxBuffPtr[temp]);
						huart3->pRxBuffPtr[temp] = 0;
					}	
				}
			 }
		}
		else {printf("command is too long \r\n"); };	
		
		cmd_interp();
		
		__HAL_UART_CLEAR_FLAG(huart3, UART_FLAG_RXNE);
		
	};
	huart3->State = HAL_UART_STATE_READY;	
	__HAL_UART_CLEAR_FLAG(huart3, UART_FLAG_IDLE);
	__HAL_UART_CLEAR_FLAG(huart3, UART_FLAG_ORE);
	__HAL_UART_CLEAR_FLAG(huart3, UART_FLAG_NE);
	
	__HAL_UART_ENABLE_IT(huart3, UART_IT_RXNE);
}

/**
* @brief This function handles CAN2 FIFO_0 Receive (Message Pending) Interrupt.
*/

void CAN2_RX0_IRQHandler(void) {
	
	HAL_CAN_IRQHandler(hcan2);
	//printf(" CAN2 DATA %02X %02X %02X \r\n", CAN2_Rx.Data[0],CAN2_Rx.Data[1],CAN2_Rx.Data[2]);
	HAL_CAN_Receive_IT (hcan2,CAN_FIFO0);
	
}

/**
* @brief This function handles CAN2 Transmit Interrupt.
*/

void CAN2_TX_IRQHandler(void) {
	
	HAL_CAN_IRQHandler(hcan2);	
	
}

/**
* @brief This function handles ADC Interrupt.
*/

//void ADC_Update(void) {
void ADC_IRQHandler(void) {
	ADC_ChannelConfTypeDef ADC_Channel_Struct;
	ADC_ChannelConfTypeDef *hadc_channel;
	uint16_t adc_temp16; 

		hadc_channel = &ADC_Channel_Struct;
		hadc_channel->Rank = 1;
		hadc_channel->SamplingTime = ADC_SAMPLETIME_15CYCLES; // 0x00 by default, minimum sampling time

	if (__HAL_ADC_GET_FLAG(hadc1, ADC_FLAG_EOC) && __HAL_ADC_GET_IT_SOURCE(hadc1, ADC_IT_EOC)) {

	/////////////////  
	// DC_mon UPDATE
	//////////////////	
	if (hadc1->NbrOfCurrentConversionRank == 1) {
		DC_mon = HAL_ADC_GetValue(hadc1);
		hadc_channel->Channel = ADC_CHANNEL_1;
		HAL_ADC_ConfigChannel(hadc1, hadc_channel);
	};
	
	/////////////////  
	// RF_mon UPDATE
	//////////////////	
	if (hadc1->NbrOfCurrentConversionRank == 2) {
		RF_mon = HAL_ADC_GetValue(hadc1);
		hadc_channel->Channel = ADC_CHANNEL_2;
		HAL_ADC_ConfigChannel(hadc1, hadc_channel);
	};
	
	/////////////////  
	// InnPD_mon UPDATE
	//////////////////	
	if (hadc1->NbrOfCurrentConversionRank == 3) {
		InnPD_mon = HAL_ADC_GetValue(hadc1);
		hadc_channel->Channel = ADC_CHANNEL_3;
		HAL_ADC_ConfigChannel(hadc1, hadc_channel);
		
		// Автоматическое поддержание мощности по внешнему фотодиоду
		
		if (LD_PWR_stb_activated) {
			if (!LD_PWR_stb_activated_prev) InnPD_mon_Fixed = InnPD_mon;
			
			if (InnPD_mon < InnPD_mon_Fixed) {			
				adc_temp16 = (uint16_t)(HAL_DAC_GetValue(&hdac1,DAC_CHANNEL_1));
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R, adc_temp16++);
			};
			
			if (InnPD_mon > InnPD_mon_Fixed) {
				adc_temp16 = (uint16_t)(HAL_DAC_GetValue(&hdac1,DAC_CHANNEL_1));
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R, adc_temp16--);
			};
			
		}
		LD_PWR_stb_activated_prev = LD_PWR_stb_activated;
	};
	
	/////////////////  
	// ExtPD_mon UPDATE
	//////////////////	
	if (hadc1->NbrOfCurrentConversionRank == 4) {
		ExtPD_mon = HAL_ADC_GetValue(hadc1);
		
		// ExtPD FIR Fiilter update
		adc_temp16 = (uint16_t)(ExtPD_mon/FLTR_ORDER);
		ExtPD_avg = ExtPD_avg - ExtPD_samples[ExtPD_pntr] + adc_temp16;// + 10*(adc_temp16-ExtPD_avg);
		ExtPD_samples[ExtPD_pntr] = adc_temp16;
		ExtPD_pntr++; 
		if (ExtPD_pntr == FLTR_ORDER) ExtPD_pntr = 0;
			
		// Non-blocking STABILIZATION function 
		if ( Stab_ON )	{
			if (ExtPD_mon < ExtPD_SETPOINT_DEPTH*ExtPD_outside) { 
				//Tld_set=Tld_set+(uint16_t)((1074-ExtPD_avg)/1);
				Tld_set=Tld_set+1;
				printf("+ \r\n");
				TEC_Set(Tld_set);
			}
			else 
				if(ExtPD_mon > ExtPD_SETPOINT_DEPTH*ExtPD_outside) {	
					//Tld_set=Tld_set-(uint16_t)((ExtPD_avg-1074)/2);
					printf("- \r\n");
					Tld_set=Tld_set-1;
					TEC_Set(Tld_set);
				}
		}
		// END of Non-blocking STABILIZATION function 
		
		// FLAG_CAPTURE Setting
		if ( !(status & FLAG_CAPTURE) && ( (Condition == STATE_10_CVT_STAB_WAIT) || (Condition == STATE_11_FBG_STAB_WAIT)) ) {
			if (ExtPD_mon > ExtPD_SETPOINT_DEPTH*ExtPD_outside) status |= FLAG_CAPTURE;
		}
		
		hadc_channel->Channel = ADC_CHANNEL_6;
		HAL_ADC_ConfigChannel(hadc1, hadc_channel);
	};

	/////////////////  
	// Tld_real UPDATE
	//////////////////	
	if (hadc1->NbrOfCurrentConversionRank == 5)	{
		Tld_real = HAL_ADC_GetValue(hadc1);
		hadc_channel->Channel = ADC_CHANNEL_7;
		HAL_ADC_ConfigChannel(hadc1, hadc_channel);
		
		// TEC OVERCURRENT SAFETY ROUTINE
		if (Itec > ITEC_THR ) {
			Condition_Request = STATE_1_LASER_OFF;  // switch laser to OFF state
			syserr |= SYSERR_TEC_OVRC; 							// set TEC Overcurrent Error in syserr register
		};
	};
	
	/////////////////  
	// Tld_setmon UPDATE
	//////////////////	
	if (hadc1->NbrOfCurrentConversionRank == 6) {
		Tld_setmon = HAL_ADC_GetValue(hadc1);
		hadc_channel->Channel = ADC_CHANNEL_8;
		HAL_ADC_ConfigChannel(hadc1, hadc_channel);
		
		// Equality between set and actual temeperatures control
		if ( ( (Tld_setmon - Tld_real) < TEMP_EQU_PRECISION ) | ( (Tld_real - Tld_setmon) < TEMP_EQU_PRECISION ) ) status |= FLAG_TEMP_EQU;
		else status &= ~FLAG_TEMP_EQU;
		
		// Using FLAG_TEMP_EQU as trigger for FLAG_SCAN_INIT
		if ( (Last_Condition == Condition) && ( (Condition == STATE_6_CVT_SCAN_INIT) || (Condition == STATE_7_FBG_SCAN_INIT) ) ){ 
			// проверяя Last_Condition == Condition гарантируем, что процесс выхода на точку ИМЕННО ЗАКОНЧИЛСЯ, а не НЕ НАЧИНАЛСЯ
			// во втором круге захода в состояние это гарантируется
			if ( (status & FLAG_TEMP_EQU) && (status & FLAG_SCAN_INIT) ) {
				status &= ~FLAG_SCAN_INIT;
			}
		};			
		
	};

	/////////////////  
	// T_brd UPDATE
	//////////////////	
	if (hadc1->NbrOfCurrentConversionRank == 7) {
		Tbrd = HAL_ADC_GetValue(hadc1);
		hadc_channel->Channel = ADC_CHANNEL_9;
		HAL_ADC_ConfigChannel(hadc1, hadc_channel);
	};
	
	/////////////////  
	// ITEC UPDATE
	//////////////////	
	if (hadc1->NbrOfCurrentConversionRank == 8) {
		Itec = HAL_ADC_GetValue(hadc1);
		hadc_channel->Channel = ADC_CHANNEL_0;
		HAL_ADC_ConfigChannel(hadc1, hadc_channel);
		
		// TEC OVERCURRENT SAFETY ROUTINE
		if (Itec > ITEC_THR ) {
			Condition_Request = STATE_1_LASER_OFF;  // switch laser to OFF state
			syserr |= SYSERR_TEC_OVRC; 							// set TEC Overcurrent Error in syserr register
		};
	}
	
	////////////////////////////////////////////////  
	// Current of Current Conversion Rank INCREMENT
	////////////////////////////////////////////////		
	hadc1->NbrOfCurrentConversionRank++;
		if (hadc1->NbrOfCurrentConversionRank > 8) { 
		hadc1->NbrOfCurrentConversionRank = 1;
		//HAL_ADC_Stop_IT(hadc1);
	}
	
	__HAL_ADC_CLEAR_FLAG(hadc1, ADC_FLAG_EOC);
	}
	
	
	if (__HAL_ADC_GET_FLAG(hadc1, ADC_FLAG_OVR) && __HAL_ADC_GET_IT_SOURCE(hadc1, ADC_IT_OVR)) {
	
		printf("OVR on Rank=%01d \r\n",hadc1->NbrOfCurrentConversionRank);
		__HAL_ADC_CLEAR_FLAG(hadc1, ADC_FLAG_OVR);
	}
}

/*-----------------------------------------------------------------------------
  ADC1 DMA Handler
 *----------------------------------------------------------------------------*/

void HAL_ADC_ConvCpltCallback( ADC_HandleTypeDef * hadc){
    printf("ADC conversion done \r\n");
    HAL_ADC_Stop_DMA(hadc);
    HAL_ADC_Stop(hadc);
}

void EXTI4_IRQHandler(void) {
		
	if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4) != RESET)
  { 
		//HAL_UART_DeInit(huart3);
		LED_On(1);
		//HAL_Delay(1000);
		//HAL_UART_Init(huart3);
		//HAL_Delay(200);
		LED_Off(1);  
    //HAL_GPIO_EXTI_Callback(GPIO_PIN_4);
		
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);
  }
		//HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
		
		#ifdef _UART_CONSOLE_FULL
			printf(" *** UART Reloaded *** ");
		#endif
}
