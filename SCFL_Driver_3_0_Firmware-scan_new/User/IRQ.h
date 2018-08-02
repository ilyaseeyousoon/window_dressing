// Header: SFCL_Driver_3_0 Firmware v1.0
// File Name: IRQ.h
// Abstract: Interrupt Handlers
// Author: D. Shelestov
// Date: 07.06.2015

/*-----------------------------------------------------------------------------
  Includes
 *----------------------------------------------------------------------------*/

#include "Main.h"
#include "Init.h"
#include "stm32f2xx_hal.h"
#include "stm32f2xx_hal_def.h"
#include "stm32f2xx_hal_conf.h"
#include "stm32f2xx_hal_gpio.h"
#include "stm32f2xx_hal_uart.h"
#include "stm32f2xx_hal_spi.h"
#include "stm32f2xx_hal_adc.h"
#include "stm32f2xx_hal_dac.h"
#include "stm32f2xx_hal_can.h"
#include "stdio.h"
#include <stdbool.h>

/*-----------------------------------------------------------------------------
  Global Variables from MAIN.c
 *----------------------------------------------------------------------------*/
/* Handles and Buffers*/
extern uint8_t UART_tx_buf[];
extern uint8_t UART_rx_buf[];
extern UART_HandleTypeDef *huart3;

extern uint8_t SPI_tx_buf[];
extern uint8_t SPI_rx_buf[];
extern SPI_HandleTypeDef hspi3;

extern DAC_HandleTypeDef hdac1;
extern ADC_HandleTypeDef *hadc1;

extern CAN_HandleTypeDef *hcan2;
extern CanTxMsgTypeDef CAN2_Tx, CAN2_Tx2;
extern CanRxMsgTypeDef CAN2_Rx;


/* Constants */
//extern const uint8_t str_length;
extern const uint8_t STR_LENGTH_MAX;
extern const uint8_t COMM_WIDTH;
extern const uint8_t FLTR_ORDER;

/* Laser State Variables */
extern CMD_TypeDef cmd; // Current command
extern uint8_t 	syserr; // 8 bits System Errors Byte  
extern uint16_t status; // 16 bits STATUS Byte  
extern uint32_t main_err; // temporary storage for function errors
extern uint8_t Condition;						// переменная для хранения текущего состояния
extern uint8_t Last_Condition;						// переменная для хранения предыдущего состояния
extern uint8_t Condition_Request;		// переменная для хранения нового необработанного состояния

/* Laser Parameters */
extern uint16_t DC_set; 		// LD POWER value
extern uint16_t DC_mon;			// Voltage from LD current on DC channel ()
extern uint16_t RF_mon;			// Voltage from LD current on AC channel ()
extern uint16_t InnPD_mon;	// Voltage on channel for Photodiode Embedded in Butterfly
extern uint16_t InnPD_mon_Fixed;  // рабочая точка по мощности на внутр фотодиоде
extern uint16_t ExtPD_mon;	// Voltage on External Photodiode channel
extern uint16_t ExtPD_avg;  // Voltage on External Photodiode channel after internal filtration 
extern uint16_t ExtPD_outside; // Voltage on External Photodiode channel after internal filtration fixed in the start of 'scan' function
extern uint16_t ExtPD_samples[];
extern uint8_t  ExtPD_pntr;

extern uint16_t Tld_set;		// LD Temperature Setpoint
extern uint16_t Tld_setmon; // Voltage on LD Temperature Setpoint
extern uint16_t Tld_real;		// Real LD Temperature
extern uint16_t Tbrd;				// Board Temperature
extern uint16_t Itec;				// TEC Current monitor
extern bool TEC_T_Flag;  		// Over-Temperature flag on TEC Driver (OPA569), 0 - temp ok, 1 - overheat happened
extern bool SW_mon; 		  	// Get Optical Switch feedback: pos1 or pos2 
extern bool SW_set;					// Set state of Optical Switch: 0 - pos1, 1 - pos2 
extern bool Stab_ON;				/* Cuvette/Bragg Grating Stabilization Enable signal: 0 - OFF, 1 - ON.
														Realization of stabilization process in NON-BLOCKING mode */
extern bool LD_CR_PWR_stb; // переключатель стабилизации лазерного диода по току или по выходной мощности с внутр. фотодиода
extern bool LD_PWR_stb_activated; 			// непрерывное поддержание мощности на внутр.фотодиоде (используется в обработчике от АЦП)
extern bool LD_PWR_stb_activated_prev;

/*-----------------------------------------------------------------------------
  Handle Functions in IRQ.c
 *----------------------------------------------------------------------------*/

void SysTick_Handler(void);
void USART3_IRQHandler(void);
void CAN2_RX0_IRQHandler(void);
void CAN2_TX_IRQHandler(void);
void ADC_Update(void);
void HAL_ADC_ConvCpltCallback( ADC_HandleTypeDef * hadc);
void EXTI4_IRQHandler(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
