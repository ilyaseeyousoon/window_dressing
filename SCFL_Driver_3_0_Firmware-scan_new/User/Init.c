// Header: SFCL_Driver_3_0 Firmware v1.0
// File Name: Init.c
// Abstract: Separate INIT functions for all blocks which is needed 
// Author: D. Shelestov
// Date: 07.05.2015

#include "init.h"

/* consts from MAIN.C */

extern const uint8_t STR_LENGTH_MAX;
extern const uint8_t COMM_WIDTH;

/* vars from MAIN.C */

extern uint8_t UART_tx_buf[];
extern uint8_t UART_rx_buf[];
extern UART_HandleTypeDef *huart3;

extern uint8_t SPI_tx_buf[];
extern uint8_t SPI_rx_buf[];
extern SPI_HandleTypeDef hspi3;

extern DAC_HandleTypeDef hdac1;

extern ADC_HandleTypeDef *hadc1;

extern DMA_HandleTypeDef *hdma1;

extern CAN_HandleTypeDef *hcan2;
extern CanTxMsgTypeDef CAN2_Tx, CAN2_Tx2;
extern CanRxMsgTypeDef CAN2_Rx;

extern FLASH_EraseInitTypeDef Flash; 
extern uint32_t FLASH_delay_s;  // short delay
extern uint32_t FLASH_delay_l; // long delay
	

extern uint16_t CAN_freq;
extern uint16_t FLASH_test_byte;

// LD Power/Current variables
extern uint16_t DC_set_c; // Default LD POWER value  (Loaded from FLASH)
extern uint16_t DC_set_p; // Default LD POWER value
extern uint16_t DC_set_max; // Power HIgh limit (Loaded from FLASH)
extern uint16_t DC_set_min;
extern uint16_t Tld_set_min;
extern uint16_t Tld_set_max;


extern uint16_t Tld_set;
extern uint16_t DC_mon;
extern uint16_t RF_mon;
extern uint16_t InnPD_mon;


extern bool TEC_EN;
extern bool LD_CR_PWR_stb; // переключатель стабилизации лазерного диода по току или по выходной мощности с внутр. фотодиода
extern bool LD_PWR_stb_activated; // непрерывное поддержание мощности на внутр.фотодиоде (используется в обработчике от АЦП)

extern volatile uint32_t adcData[8];

/* Laser State Variables */
extern CMD_TypeDef cmd; // Current command
extern uint8_t syserr; // 8 bits System Errors Byte  
extern int16_t status; // 16 bits STATUS Byte  СО СТАТУСОМ ВОЗНИК КОНФЛИКТ.НАДО МЕНЯТЬ НАЗВАНИЕ
extern uint32_t main_err; // temporary storage for function errors
extern uint32_t Sector_Error; // Error register for FLASH


extern uint8_t tec_work;
extern uint16_t tec_value_old,tec_value_new;

/**
  \fn          int32_t LED_Initialize (void)
  \brief       Initialize LEDs
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t ALL_PINS_Initialize (void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
	__GPIOB_CLK_ENABLE();
  __GPIOD_CLK_ENABLE();
  
  /* Configure GPIO output pins: 
		 PD1 - TEC Enable pin	
		 PD2, PD3 - LEDs
		 PB8 - SW_set
	*/
  GPIO_InitStruct.Pin   = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin   = GPIO_PIN_8;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* Configure GPIO input pins: 
		 PD0 - TEC Thermal Flag
		 PD4(S4), PD5(S3) - Buttons
		 PB9 - SW_mon
	*/
  GPIO_InitStruct.Pin   = GPIO_PIN_4;
  GPIO_InitStruct.Mode  = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  
	GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_5;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin   = GPIO_PIN_9;
//	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Pull  = GPIO_PULLDOWN;// сделал подтяжуку к земле. не знаю,можно ли
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
		
  return 0;
}

/**
  \fn          int32_t LED_Uninitialize (void)
  \brief       De-initialize LEDs
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t LED_Uninitialize (void) {

  HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2 | GPIO_PIN_3);
  
  return 0;
}

/**
  \fn          int32_t LED_On (uint32_t num)
  \brief       Turn on requested LED
  \param[in]   num  LED number
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t LED_On (uint32_t num) {
  HAL_GPIO_WritePin(LED_PIN[num].port, LED_PIN[num].pin, GPIO_PIN_SET);
  return 0;
}

/**
  \fn          int32_t LED_Off (uint32_t num)
  \brief       Turn off requested LED
  \param[in]   num  LED number
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t LED_Off (uint32_t num) {
  HAL_GPIO_WritePin(LED_PIN[num].port, LED_PIN[num].pin, GPIO_PIN_RESET);
  return 0;
}

/**
  \fn          int32_t TEC_On (uint32_t num)
  \brief       Turn on TEC OpAmps
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t TEC_On (void) {
  HAL_GPIO_WritePin(ALL_PIN[1].port, ALL_PIN[1].pin, GPIO_PIN_SET);
  return 0;
}

/**
  \fn          int32_t TEC_Off (uint32_t num)
  \brief       Turn off TEC OpAmps
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t TEC_Off (void) {
  HAL_GPIO_WritePin(ALL_PIN[1].port, ALL_PIN[1].pin, GPIO_PIN_RESET);
  return 0;
}

/**
  \fn          int32_t TEC_Set (uint16_t setpoint)
  \brief       Sets requested temperature value 
  \param[in]   setpoint 16-bit temperature point
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t TEC_Set (uint16_t setpoint) {
	#ifdef DAC_EX_INSTALLED
	tec_value_new=setpoint;
	tec_value_old = (uint16_t)(HAL_DAC_GetValue(&hdac1,DAC_CHANNEL_2));	
	tec_work=1;
#else
	tec_value_new=setpoint;
	tec_value_old = (uint16_t)(HAL_DAC_GetValue(&hdac1,DAC_CHANNEL_2));	
	tec_work=1;
	#endif
		

  return 0;
}

/**
  \fn          int32_t LD_On (uint16_t setpoint)
  \brief       Smooth switch the laser power (=current) from previous value to new setpoint
  \param[in]   new setpoint (normally it's DC-SET variable)
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/

int32_t LD_On (uint16_t c_setpoint, uint16_t p_setpoint) {
	uint16_t tmp; 
	uint16_t delay_value = 7; // for simulation->  =1;
	uint16_t old_set;
	uint32_t tick_start;
	uint32_t tick_delta = 50000;
	cmd.State = CMD_STATE_BUSY;
	tick_start = HAL_GetTick();
	printf("WAIT           ");
	old_set = (uint16_t)(HAL_DAC_GetValue(&hdac1,DAC_CHANNEL_1));	
	
	/****************************************************************/
	/*  Режим стабилизации тока лазерного диода (т.е. НЕ мощности)  */
	/****************************************************************/	
	if (!LD_CR_PWR_stb) {
		
		LD_PWR_stb_activated = DISABLE; // НА всякий случай выключаем поддержание мощности в АЦП обработчике, хотя он и так должен быть тут выключен.
	
		if (old_set < c_setpoint)
			for (tmp = old_set; tmp < c_setpoint; tmp++)	{
				HAL_Delay(delay_value);
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R, tmp);
			
				if (( HAL_GetTick() - tick_start) > tick_delta) {   
					printf("TIMEOUT: impossible to set LD power");
					status &= ~FLAG_PWR_UPDOWN;  // сброс флага переменной мощности
						cmd.State = CMD_STATE_READY;
					return 1;
				}
				
//				 printf(" %10x FLAG_PWR_UPDOWN \r\n", status);
			};
			
		if (old_set > c_setpoint)
			for (tmp = old_set; tmp > c_setpoint; tmp--)	{
				HAL_Delay(delay_value);
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R, tmp);
				if (( HAL_GetTick() - tick_start) > tick_delta) {   
					printf("TIMEOUT: impossible to set LD power");
					status &= ~FLAG_PWR_UPDOWN;  // сброс флага переменной мощности
						cmd.State = CMD_STATE_READY;
					return 1;
				}
			};
		status &= ~FLAG_PWR_UPDOWN;  // сброс флага переменной мощности
			cmd.State = CMD_STATE_READY;	
	}
	
	/****************************************************************/
	/* Режим стабилизации мощности лазерного диода (т.е. НЕ тока)   */
	/****************************************************************/	
	else {
		
		LD_PWR_stb_activated = DISABLE; // Выключаем поддержание мощности, чтобы АЦП процесс не вмешивался в изменение мощности	
		
		if (InnPD_mon < p_setpoint)
			for (tmp = old_set; InnPD_mon < p_setpoint; tmp++)	{
				HAL_Delay(delay_value);
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R, tmp);
			
				if (( HAL_GetTick() - tick_start) > tick_delta) {   
					printf("TIMEOUT: impossible to set LD power");
					status &= ~FLAG_PWR_UPDOWN;  // сброс флага переменной мощности
						cmd.State = CMD_STATE_READY;
					return 1;
				}
			};
			
		if (InnPD_mon > p_setpoint)
			for (tmp = old_set; InnPD_mon > p_setpoint; tmp--)	{
				HAL_Delay(delay_value);
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R, tmp);
				if (( HAL_GetTick() - tick_start) > tick_delta) {   
					printf("TIMEOUT: impossible to set LD power");
					status &= ~FLAG_PWR_UPDOWN;  // сброс флага переменной мощности
						cmd.State = CMD_STATE_READY;
					return 1;
				}
			};
		status &= ~FLAG_PWR_UPDOWN;  // сброс флага переменной мощности
				cmd.State = CMD_STATE_READY;
		LD_PWR_stb_activated = ENABLE; // Включаем поддержание мощности, чтобы АЦП процесс захватил зафиксировал выставленную точку
	}
		
	cmd.State = CMD_STATE_READY;
  return 0;
}

/**
  \fn          int32_t LD_Off (void)
  \brief       Smooth switch off the laser power
  \param[in]   none
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/

int32_t LD_Off (void) {
	
	LD_On(0,0);
	
  return 0;
}




/**
  \fn          int32_t UART3_Initialize (void)
  \brief       Initialize UART3 on PD8-PD97 ports
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
	
	UART_InitTypeDef UART_InitStruct;
	GPIO_InitTypeDef GPIO_InitStruct;
  
	/* GPIO Ports Clock Enable */
  __GPIOD_CLK_ENABLE();
  
  /* Configure GPIO pins: PD8 - from STM to PC,  PD9 - from PC to STM */
  GPIO_InitStruct.Pin   = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
	GPIO_InitStruct.Alternate =	GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	/* USART Clock Enable */
  __USART3_CLK_ENABLE();
	
	/* Configure UART Init */
	UART_InitStruct.BaudRate = 256000;
  UART_InitStruct.WordLength = UART_WORDLENGTH_8B;  /* default */                
  UART_InitStruct.StopBits = UART_STOPBITS_1; /* default */
  UART_InitStruct.Parity = UART_PARITY_NONE;  /* default */                 
  UART_InitStruct.Mode = UART_MODE_TX_RX;
  
  /* Configure USART Handler */	  
	huart->Instance = USART3;
	huart->Init = UART_InitStruct;
	huart->pTxBuffPtr = &UART_tx_buf[0];
	huart->TxXferSize = STR_LENGTH_MAX; 
	huart->TxXferCount = 0;
	huart->pRxBuffPtr = &UART_rx_buf[0];
	huart->RxXferSize = STR_LENGTH_MAX;
	huart->RxXferCount = 0;
	huart->Lock = HAL_UNLOCKED;
	huart->State = HAL_UART_STATE_RESET;
	huart->ErrorCode = HAL_UART_ERROR_NONE;
	
}

/**
  \fn          void HAL_CAN_MspInit ()
  \brief       Initialize CAN2 on PB5 RX and PB6 TX ports
*/
void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan) {
	
	CAN_InitTypeDef CAN_InitStruct;
	CAN_FilterConfTypeDef CAN_Filter;
	GPIO_InitTypeDef GPIO_InitStruct;
  
	/* GPIO Ports Clock Enable */
  __GPIOB_CLK_ENABLE();
  
  /* Configure GPIO pins: PB5 RX - from OUTER to LASER,  PB6 TX - from LASER to OUTER */
  GPIO_InitStruct.Pin   = GPIO_PIN_5 | GPIO_PIN_6;
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Alternate =	GPIO_AF9_CAN2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/* CAN2 Clock Enable */
	__CAN1_CLK_ENABLE();
  __CAN2_CLK_ENABLE();
	
	/* Configure CAN Init */      
	CAN_InitStruct.Mode = CAN_MODE_NORMAL; // 0 - by default
	switch (CAN_freq) {
		case 125:
			printf(" CAN freq = 125 kHz \r\n");
			CAN_InitStruct.Prescaler = 10;
			CAN_InitStruct.SJW = CAN_SJW_1TQ; 
			CAN_InitStruct.BS1 = CAN_BS1_15TQ; 
			CAN_InitStruct.BS2 = CAN_BS2_8TQ; // Total CAN BaudRate 125 kHz
		break;
		case 250:
			printf(" CAN freq = 250 kHz \r\n");
			CAN_InitStruct.Prescaler = 5;
			CAN_InitStruct.SJW = CAN_SJW_1TQ; 
			CAN_InitStruct.BS1 = CAN_BS1_15TQ; 
			CAN_InitStruct.BS2 = CAN_BS2_8TQ; // Total CAN BaudRate 250 kHz
		break;
		case 500:
			printf(" CAN freq = 500 kHz \r\n");
			CAN_InitStruct.Prescaler = 5;
			CAN_InitStruct.SJW = CAN_SJW_1TQ; 
			CAN_InitStruct.BS1 = CAN_BS1_7TQ; 
			CAN_InitStruct.BS2 = CAN_BS2_4TQ; // Total CAN BaudRate 500 kHz
		break;
		case 1000:
			printf(" CAN freq = 1 MHz \r\n");
			CAN_InitStruct.Prescaler = 3;
			CAN_InitStruct.SJW = CAN_SJW_1TQ; 
			CAN_InitStruct.BS1 = CAN_BS1_6TQ; 
			CAN_InitStruct.BS2 = CAN_BS2_3TQ; // Total CAN BaudRate 1000 kHz
		break;
		default:
			printf("Incorrect CAN value in FLASH replaced by default 125 kHz \r\n");
			CAN_InitStruct.Prescaler = 10;
			CAN_InitStruct.SJW = CAN_SJW_1TQ; 
			CAN_InitStruct.BS1 = CAN_BS1_15TQ; 
			CAN_InitStruct.BS2 = CAN_BS2_8TQ; // Total CAN BaudRate 125 kHz
		break;
	}
	CAN_InitStruct.TTCM = DISABLE; // 0 - by default
	CAN_InitStruct.ABOM = DISABLE; // 0 - by default
	CAN_InitStruct.AWUM = DISABLE; // 0 - by default
	CAN_InitStruct.NART = DISABLE; // 0 - by default
	CAN_InitStruct.RFLM = DISABLE; // 0 - by default
	CAN_InitStruct.TXFP = DISABLE; // 0 - by default
	
	/* Configure CAN Filter */     
	CAN_Filter.FilterIdHigh = 0;
	CAN_Filter.FilterIdLow = 0;
	CAN_Filter.FilterMaskIdHigh = 0;
	
	CAN_Filter.FilterMaskIdLow = 0;
	CAN_Filter.FilterFIFOAssignment = CAN_FILTER_FIFO0; // 0 - by default
	CAN_Filter.FilterNumber = 0;
	CAN_Filter.FilterMode = CAN_FILTERMODE_IDMASK; // 0 - by default
	CAN_Filter.FilterScale = CAN_FILTERSCALE_32BIT; // 0 - by default
	CAN_Filter.FilterActivation = ENABLE;
	CAN_Filter.BankNumber = 0;
	
	/* Configure CAN Tx Message */     
	CAN2_Tx.StdId = 1;
	CAN2_Tx.ExtId = 0;
	CAN2_Tx.IDE = CAN_ID_STD; // 0 - by default
	CAN2_Tx.RTR = CAN_RTR_DATA; // 0 - by default
	CAN2_Tx.DLC = 3;
	CAN2_Tx.Data[0] = 0xFF;
	CAN2_Tx.Data[1] = 0xAA; 	
	CAN2_Tx.Data[2] = 0x55;
	
	CAN2_Tx2.StdId = 1;
	CAN2_Tx2.ExtId = 0;
	CAN2_Tx2.IDE = CAN_ID_STD; // 0 - by default
	CAN2_Tx2.RTR = CAN_RTR_DATA; // 0 - by default
	
	/* Configure CAN Rx Message */     
//	CAN2_Rx.StdId = 1;
//	CAN2_Rx.ExtId = 0;
//	CAN2_Rx.IDE = CAN_ID_STD; // 0 - by default
//	CAN2_Rx.RTR = CAN_RTR_DATA; // 0 - by default
//	CAN2_Rx.DLC = 8;
	CAN2_Rx.FMI = 14;
	CAN2_Rx.FIFONumber = CAN_FIFO0;
	
  /* Configure CAN Handler */	  
	hcan->Instance = CAN2;
	hcan->Init = CAN_InitStruct;
	hcan->pTxMsg = &CAN2_Tx;
	hcan->pRxMsg = &CAN2_Rx;
	hcan->Lock = HAL_UNLOCKED;
	hcan->State = HAL_CAN_STATE_RESET;
	hcan->ErrorCode = HAL_CAN_ERROR_NONE;
	
	//HAL_CAN_Init(hcan);
	HAL_CAN_ConfigFilter(hcan, &CAN_Filter);
	
}

/**
  \fn          int32_t SPI3_Initialize (void)
  \brief       Initialize USART6 on PC6-PC7 ports
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t SPI3_Initialize (void) {
  
	SPI_InitTypeDef SPI_InitStruct;
	GPIO_InitTypeDef GPIO_InitStruct;
  
	/* GPIO Ports Clock Enable */
  __GPIOC_CLK_ENABLE();
	__GPIOD_CLK_ENABLE();
  
  /* Configure GPIO pins: PC6 - from STM to PC,  PC7 - from PC to STM */
  GPIO_InitStruct.Pin   = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
	GPIO_InitStruct.Alternate =	GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	/* Configure GPIO pins: PD2 - SS Signal */
  GPIO_InitStruct.Pin   = GPIO_PIN_2;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
	
	/* Configure GPIO pins: PD2 - SS Signal */
  GPIO_InitStruct.Pin   = GPIO_PIN_7;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
	
	/* SPI3 Clock Enable */
  __SPI3_CLK_ENABLE();
	
	/* Configure SPI Init */      
  SPI_InitStruct.Mode = SPI_MODE_MASTER;
	SPI_InitStruct.Direction = SPI_DIRECTION_2LINES; /* default */
	SPI_InitStruct.DataSize = SPI_DATASIZE_16BIT;
	SPI_InitStruct.CLKPolarity = SPI_POLARITY_LOW;
	SPI_InitStruct.CLKPhase = SPI_PHASE_2EDGE; /* default */
	SPI_InitStruct.NSS = SPI_NSS_SOFT;
	SPI_InitStruct.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	SPI_InitStruct.FirstBit = SPI_FIRSTBIT_MSB; /* default */
	SPI_InitStruct.TIMode = SPI_TIMODE_DISABLED; /* default */
	SPI_InitStruct.CRCCalculation = SPI_CRCCALCULATION_DISABLED; /* default */
	SPI_InitStruct.CRCPolynomial = 0; /* default, no need without CRC */
		
  /* Configure SPI Handler */	  
	hspi3.Instance = SPI3;
	hspi3.Init = SPI_InitStruct;
	hspi3.pRxBuffPtr = &SPI_tx_buf[0];
	hspi3.TxXferSize = 1;
	hspi3.TxXferCount = 1;
	hspi3.pRxBuffPtr = &SPI_rx_buf[0];
	hspi3.RxXferSize = 1;
	hspi3.RxXferCount = 1;
	hspi3.Lock = HAL_UNLOCKED;
	hspi3.State = HAL_SPI_STATE_RESET;
	hspi3.ErrorCode = HAL_SPI_ERROR_NONE;
	
	HAL_SPI_Init(&hspi3);
	
  return 0;
}


int32_t DAC_Initialize (void) {
  GPIO_InitTypeDef GPIO_InitStruct;
	DAC_ChannelConfTypeDef DAC_Channel_Struct;
	
	uint32_t temp32 = 0; // initial value for DAC

  /* GPIO Ports and DAC Clock Enable */
  __GPIOA_CLK_ENABLE();
	__DAC_CLK_ENABLE();
  
  /* Configure GPIO pins: PA4 - DAC1 (Diode Power), PA5 - DAC2 (TEC temperature) */
  GPIO_InitStruct.Pin   = GPIO_PIN_4 | GPIO_PIN_5;
  GPIO_InitStruct.Mode  = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		
	/* Configure DAC Handler*/
	hdac1.Instance = DAC;
	hdac1.State = HAL_DAC_STATE_RESET; // by default 
	hdac1.Lock = HAL_UNLOCKED; // by default

	HAL_DAC_Init(&hdac1);
  
	/* Configure DAC Channels */
	DAC_Channel_Struct.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE; // by default
	DAC_Channel_Struct.DAC_Trigger = DAC_TRIGGER_NONE; // by default
	
	HAL_DAC_ConfigChannel(&hdac1,&DAC_Channel_Struct,DAC_CHANNEL_1); // DAC1 = Laser Diode Power
	HAL_DAC_ConfigChannel(&hdac1,&DAC_Channel_Struct,DAC_CHANNEL_2); // DAC2 = TEC Temperature Setpoint	

	temp32 = 0; // zero power while start driver
	HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R, temp32);
	temp32 = Tld_set; // half of voltage range to have temperature in the middle
	HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_2,DAC_ALIGN_12B_R, temp32);

	/* Launch DAC both channels independently */
	HAL_DAC_Start(&hdac1,DAC_CHANNEL_1);
	HAL_DAC_Start(&hdac1,DAC_CHANNEL_2);
			
  return 0;
}


int32_t ADC_Initialize(void) {
	GPIO_InitTypeDef GPIO_InitStruct;
	ADC_InitTypeDef ADC_InitStruct;
	
	ADC_ChannelConfTypeDef ADC_Channel_Struct;
	ADC_ChannelConfTypeDef *hadc_channel;
	
	hadc_channel = &ADC_Channel_Struct;
	
	/* GPIO Port and ADC1 Clock Enable */
  __GPIOA_CLK_ENABLE();
	__GPIOB_CLK_ENABLE();
	__ADC1_CLK_ENABLE();

	/* Configure GPIO pins: 
		PA0 -	ADC1 IN0 = Laser Diode DC Current
		PA1 - ADC1 IN1 = Laser Diode RF Current
		PA2 - ADC1 IN2 = Laser Diode Inner Photodiode Current
		PA3 - ADC1 IN3 = Laser Diode Temperature ( Tld_real )
		PA6 - ADC1 IN6 = Laser Diode Inner Photodiode Current
		PA7 - ADC1 IN7 = Tld-set monitor V-TMP-SET_OUT
		PB0 - ADC1 IN8 = Board Temperature
		PB1 - ADC1 IN9 = ITEC TEC Current Monitor 	*/
	
  GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode  = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode  = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
			
	/* Configure ADC Init Struct*/
  ADC_InitStruct.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;	 // 0x00 by default - 120MHz/2 = 60 MHz
  ADC_InitStruct.Resolution = ADC_RESOLUTION12b; // 0x00 by default
	ADC_InitStruct.DataAlign = ADC_DATAALIGN_RIGHT; // 0x00 by default
	ADC_InitStruct.ScanConvMode = DISABLE; // 0x01  
	ADC_InitStruct.EOCSelection = EOC_SEQ_CONV; // 0x00 EOC flag after all sequence
	ADC_InitStruct.ContinuousConvMode = DISABLE; // 0x00 No continuous mode
	ADC_InitStruct.DMAContinuousRequests = DISABLE;
	ADC_InitStruct.NbrOfConversion = 1;
	ADC_InitStruct.DiscontinuousConvMode = DISABLE; // 0x00 No discontinuous mode 
	ADC_InitStruct.NbrOfDiscConversion = 0;
	ADC_InitStruct.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE; // 0x00 Ext trig not used
	ADC_InitStruct.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1; // 0x00 by default - Timer 1 CC1 as a trigger source	
	
	/* Configure ADC Handler*/
	hadc1->Instance = ADC1;
	hadc1->Init = ADC_InitStruct;
	hadc1->Lock = HAL_UNLOCKED;
	hadc1->NbrOfCurrentConversionRank = 1;
	hadc1->State = HAL_ADC_STATE_RESET;
	
	HAL_ADC_Init(hadc1); 
	
	/* Configure ADC Channels Struct*/
	// Rank 1 - ADC1 IN0 = Laser Diode DC Current
  hadc_channel->Channel = ADC_CHANNEL_0;
	hadc_channel->Rank = 1;
	hadc_channel->SamplingTime = ADC_SAMPLETIME_15CYCLES; // 0x00 by default, minimum sampling time
	HAL_ADC_ConfigChannel(hadc1, hadc_channel);
//	// Rank 2 - ADC1 IN1 = Laser Diode RF Current
//	hadc_channel->Channel = ADC_CHANNEL_1;
//	hadc_channel->Rank = 2;
//	hadc_channel->SamplingTime = ADC_SAMPLETIME_3CYCLES;
//	HAL_ADC_ConfigChannel(hadc1, hadc_channel);
//	// Rank 3 - ADC1 IN2 = Laser Diode Inner Photodiode Current
//	hadc_channel->Channel = ADC_CHANNEL_2;
//	hadc_channel->Rank = 3;
//	hadc_channel->SamplingTime = ADC_SAMPLETIME_3CYCLES;
//	HAL_ADC_ConfigChannel(hadc1, hadc_channel);
//	// Rank 4 - ADC1 IN3 = Laser Diode Temperature ( Tld_real )
//	hadc_channel->Channel = ADC_CHANNEL_3;
//	hadc_channel->Rank = 4;
//	hadc_channel->SamplingTime = ADC_SAMPLETIME_3CYCLES;
//	HAL_ADC_ConfigChannel(hadc1, hadc_channel);
//	// Rank 5 - ADC1 IN6 = Laser Diode Inner Photodiode Current
//	hadc_channel->Channel = ADC_CHANNEL_6;
//	hadc_channel->Rank = 5;
//	hadc_channel->SamplingTime = ADC_SAMPLETIME_3CYCLES;
//	HAL_ADC_ConfigChannel(hadc1, hadc_channel);
//	// Rank 6 - ADC1 IN7 = Tld-set monitor V-TMP-SET_OUT
//	hadc_channel->Channel = ADC_CHANNEL_7;
//	hadc_channel->Rank = 6;
//	hadc_channel->SamplingTime = ADC_SAMPLETIME_3CYCLES;
//	HAL_ADC_ConfigChannel(hadc1, hadc_channel);
//	// Rank 7 - ADC1 IN8 = Board Temperature
//	hadc_channel->Channel = ADC_CHANNEL_8;
//	hadc_channel->Rank = 7;
//	hadc_channel->SamplingTime = ADC_SAMPLETIME_3CYCLES;
//	HAL_ADC_ConfigChannel(hadc1, hadc_channel);
//	// Rank 8 - ADC1 IN9 = ITEC TEC Current Monitor 
//	hadc_channel->Channel = ADC_CHANNEL_9;
//	hadc_channel->Rank = 8;
//	hadc_channel->SamplingTime = ADC_SAMPLETIME_3CYCLES;
//	HAL_ADC_ConfigChannel(hadc1, hadc_channel);

		HAL_ADC_Start(hadc1);
	
//  HAL_ADC_Start_DMA(hadc1, (uint32_t*)adcData, 8);
	
	return 0;
}

// int32_t DMA_Initialize(void) {
//	DMA_InitTypeDef DMA_InitStruct;
//	
//	/* DMA1 Clock Enable */
//	__DMA1_CLK_ENABLE();
//	
//	/* Configure DMA1 InitStruct*/
//	
//	DMA_InitStruct.Channel = DMA_CHANNEL_0; // 0x00 By default
//	DMA_InitStruct.Direction = DMA_PERIPH_TO_MEMORY; // 0x00 By default
//	DMA_InitStruct.PeriphInc = DMA_PINC_DISABLE; // 0x00 by default
//	DMA_InitStruct.MemInc = DMA_MINC_DISABLE; // 0x00 by default
//	DMA_InitStruct.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; // Peripheral data alignment: HalfWord
//	DMA_InitStruct.MemDataAlignment = DMA_MDATAALIGN_HALFWORD; // Memory data alignment: HalfWord
//	DMA_InitStruct.Mode = DMA_CIRCULAR; // 0x00 by default
//	DMA_InitStruct.Priority = DMA_PRIORITY_LOW; // 0x00 by default
//	DMA_InitStruct.FIFOMode = DMA_FIFOMODE_DISABLE; // 0x00 by default
//	DMA_InitStruct.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL; // 0x00 by default
//	DMA_InitStruct.MemBurst = DMA_MBURST_INC8; // 0x00 by default
//	DMA_InitStruct.PeriphBurst = DMA_PBURST_INC8; // 0x00 by default
//	
//	/* Configure DMA1 Handler*/	
//	
//	hdma1->Instance = DMA1_Stream0;
//	hdma1->Init =	DMA_InitStruct;
//	hdma1->Lock = HAL_UNLOCKED;
//	hdma1->State = HAL_DMA_STATE_RESET;
//	//hdma1->Parent = 
//	
//	HAL_DMA_Init(hdma1);
//	HAL_DMA_Start_IT(hdma1, &(hadc1->Instance->DR) , &adcData[0], 8);
//	//__HAL_LINKDMA(hadc1,DMA_Handle,hdma_adc1);
//	//HAL_DMA_Start(hdma1,);
//	
//	return 0;
//}

int32_t IRQ_Initialize(void) {
	
	//HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4); // by default
	HAL_NVIC_SetPriority(EXTI4_IRQn, 3, 0); // important to make allow HAL_Delay from USART3 ISR
	HAL_NVIC_SetPriority(USART3_IRQn, 2, 0); // important to make allow HAL_Delay from USART3 ISR
	HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 2, 0); // important to make allow HAL_Delay from USART3 ISR
	HAL_NVIC_SetPriority(CAN2_TX_IRQn, 2, 0); // important to make allow HAL_Delay from USART3 ISR
	HAL_NVIC_SetPriority(ADC_IRQn, 1, 0); // important to make allow HAL_Delay from any ISR
	//HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
		
	HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
	HAL_CAN_Receive_IT (hcan2,CAN_FIFO0);
	HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
	
	__HAL_UART_ENABLE_IT(huart3, UART_IT_RXNE);
	HAL_NVIC_EnableIRQ(USART3_IRQn);
	
	
	//SYSCFG_EXTICR2_EXTI4_PD
	HAL_NVIC_EnableIRQ(EXTI4_IRQn);

	__HAL_ADC_ENABLE_IT(hadc1, ADC_IT_EOC);
	HAL_NVIC_EnableIRQ(ADC_IRQn);
	//HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 1, 0);
	//HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
	//HAL_ADC_Start_IT(hadc1);
		
	return 0;
}

/**
  \fn          int32_t FLASH_Initialize (void)
  \brief       Initialize FLASH
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/

int32_t FLASH_Initialize (void) {
	
	Flash.TypeErase=TYPEERASE_SECTORS;//заполнение структуры очистки страницы
	Flash.Sector=FLASH_SECTOR_3;
	Flash.NbSectors=1;
	Flash.VoltageRange=VOLTAGE_RANGE_2;
	
	// Первичная прошивка FLASH
	
	HAL_FLASH_Unlock();
	HAL_Delay(FLASH_delay_l);
	FLASH_test_byte = *((uint32_t *)FLASH_TEST_BYTE_ADR);
	HAL_Delay(FLASH_delay_l);
	if ( FLASH_test_byte != FLASH_TEST_BYTE_VALUE ) {
				
		// Единократная прошивка во FLASH значений по умолчанию
		HAL_FLASHEx_Erase(&Flash,&Sector_Error);	
		HAL_Delay(FLASH_delay_l);
		HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_INIT_ADR, DC_INIT_C); 
		HAL_Delay(FLASH_delay_s);
		HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MAX_ADR, DC_SET_MAX_INIT);
		HAL_Delay(FLASH_delay_s);
		HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MIN_ADR, DC_SET_MIN_INIT);
		HAL_Delay(FLASH_delay_s);
		HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEMP_INIT_ADR, TEMP_INIT);
		HAL_Delay(FLASH_delay_s);
		HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_CAN_FREQ_ADR, CAN_FREQ_INIT);
		HAL_Delay(FLASH_delay_s);
		HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEST_BYTE_ADR, FLASH_TEST_BYTE_VALUE);
		HAL_Delay(FLASH_delay_s);
		printf("One-time FLASH burning... DONE\r\n");
		
	}
		HAL_Delay(FLASH_delay_l);
	
		DC_set_c = *((uint32_t *)FLASH_DC_INIT_ADR);
		HAL_Delay(FLASH_delay_s);
		DC_set_max = *((uint32_t *)FLASH_DC_MAX_ADR);
		HAL_Delay(FLASH_delay_s);
		DC_set_min = *((uint32_t *)FLASH_DC_MIN_ADR);
		HAL_Delay(FLASH_delay_s);
		Tld_set = *((uint32_t *)FLASH_TEMP_INIT_ADR);
		HAL_Delay(FLASH_delay_s);
		CAN_freq = *((uint32_t *)FLASH_CAN_FREQ_ADR);
		HAL_Delay(FLASH_delay_s);
		FLASH_test_byte = *((uint32_t *)FLASH_TEST_BYTE_ADR);
		HAL_Delay(FLASH_delay_s);
		HAL_FLASH_Lock();
	
		Tld_set_min = 0;
		Tld_set_max = 4095;
	
		if ( FLASH_test_byte != FLASH_TEST_BYTE_VALUE ) {
			printf(" ! FLASH Read error, factory settings in use \r\n");
			syserr |= SYSERR_FLASH;
			DC_set_c = DC_INIT_C;
			DC_set_max = DC_SET_MAX_INIT;
			DC_set_min = DC_SET_MIN_INIT;
			Tld_set = TEMP_INIT;
			CAN_freq = CAN_FREQ_INIT;
			FLASH_test_byte = FLASH_TEST_BYTE_VALUE;
			Tld_set_min = 0;
			Tld_set_max = 4095;
		}
		
		printf("*********** \r\n");
		printf("| FLASH_DC_INIT = 0d%04d \r\n",DC_set_c);
		printf("| FLASH_DC_MAX = 0d%04d \r\n",DC_set_max);
		printf("| FLASH_DC_MIN = 0d%04d \r\n",DC_set_min);
		printf("| FLASH_TEMP_INIT = 0d%04d \r\n",Tld_set);
		printf("| FLASH_CAN_FREQ = 0d%04d \r\n",CAN_freq);
		printf("| FLASH_TEST_BYTE = 0d%04d \r\n",FLASH_test_byte);
		printf("*********** \r\n");
		
	return HAL_OK;
}
