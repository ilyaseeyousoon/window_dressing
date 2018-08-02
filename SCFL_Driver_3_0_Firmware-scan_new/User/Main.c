// Header: SFCL_Driver_3_0 Firmware v1.1
// File Name: Main.c
// Abstract: Main function of the project 
// Author: D. Shelestov
// Date: 07.05.2015

// Keil (Edit -> Configuration -> Editor) кодировку редактора: вместо ANSI выбрать UTF-8. 

#include "Main.h"


/*-----------------------------------------------------------------------------
  Constants
 *----------------------------------------------------------------------------*/
const uint8_t driver_serial = 0x06;
const uint8_t module_serial = 0x02;
const uint8_t software_version = 0x02;

const uint8_t STR_LENGTH_MAX = 8;
const uint8_t COMM_WIDTH = 3;
const uint8_t FLTR_ORDER = 10;

/*-----------------------------------------------------------------------------
  Global Variables
 *----------------------------------------------------------------------------*/

/* Handles */

SPI_HandleTypeDef hspi3;
UART_HandleTypeDef var_uart3;
UART_HandleTypeDef *huart3;
DAC_HandleTypeDef hdac1;
ADC_HandleTypeDef var_hadc1;
ADC_HandleTypeDef *hadc1;
DMA_HandleTypeDef var_hdma1;
DMA_HandleTypeDef *hdma1;
CAN_HandleTypeDef var_hcan2;
CAN_HandleTypeDef *hcan2;
FLASH_EraseInitTypeDef Flash;

/* Buffers */
uint8_t UART_tx_buf[STR_LENGTH_MAX];
uint8_t UART_rx_buf[STR_LENGTH_MAX];
uint8_t SPI_tx_buf[STR_LENGTH_MAX];
uint8_t SPI_rx_buf[STR_LENGTH_MAX];
CanTxMsgTypeDef CAN2_Tx, CAN2_Tx2;
CanRxMsgTypeDef CAN2_Rx;

/* External PD Filtering */
uint16_t ExtPD_samples[FLTR_ORDER];
uint8_t ExtPD_pntr;

/* Laser State Variables */
CMD_TypeDef cmd; // Current command
uint8_t syserr = 0; // 8 bits System Errors Byte  
int16_t status = 0; // 16 bits STATUS Byte  СО СТАТУСОМ ВОЗНИК КОНФЛИКТ.НАДО МЕНЯТЬ НАЗВАНИЕ
uint32_t main_err = 0; // temporary storage for function errors
uint32_t Sector_Error = 0; // Error register for FLASH
uint32_t FLASH_delay_s = 20;  // short delay
uint32_t FLASH_delay_l = 150; // long delay
	


volatile uint32_t adcData[8];

// temporary variables
uint16_t FLASH_DC_INIT;	  
uint16_t FLASH_DC_MAX;	  
uint16_t FLASH_DC_MIN;	  
uint16_t FLASH_TEMP_INIT;	
uint16_t FLASH_CAN_FREQ;	
uint16_t FLASH_CTRL_BYTE;	

// delete them

/* Main var group */
// LD Power/Current variables
uint16_t DC_set_c; // Default LD POWER value  (Loaded from FLASH)
uint16_t DC_set_p = DC_INIT_P; // Default LD POWER value
uint16_t DC_set_min; // Power LOW limit (Loaded from FLASH)
uint16_t DC_set_max; // Power HIGH limit (Loaded from FLASH)
uint16_t Tld_set_min;
uint16_t Tld_set_max;

uint16_t DC_mon;
uint16_t RF_mon;
uint16_t InnPD_mon;       // текущая мощность на внутр фотодиоде
uint16_t InnPD_mon_Fixed; // рабочая точка по мощности на внутр фотодиоде
uint16_t ExtPD_mon;
uint16_t ExtPD_avg;
uint16_t ExtPD_outside;
uint16_t Tld_set = TEMP_CVT_SCAN_INIT;

uint8_t Last_Condition	= 0;			// сохранение последнего состояния
uint8_t Condition	= 0; 						// переменная для сохранения состояния
uint8_t Condition_Request	= 0;		// переменная для хранения нового необработанного состояния
uint8_t  scan_fatality = 0; 			// переменная для подчитывания количества ошибок сканирования 
uint16_t temp_logic = 0;
uint8_t switch_error = 0;
uint8_t lakmus=0;
uint8_t scan_request=0; // если scan_request=33 в систике начнется сканирование

extern uint8_t tec_work;
extern uint16_t tec_value_old,tec_value_new;

// old_value 1568 byte 1 = 0d098 byte 2 = 0d000 // 1542 nm 
// center 1542,386 nm   J 102 004 	0d1636  Tld_real = 0d1584
// *****   Laser S/N YE84079  *************
// Acetylene S/N J 061 010 	0d0986  Tld_real = 0d1277 

//old FBG S/N 541504A003 J 055 008 0d0888 	center = 1542,405 nm
uint16_t Tld_setmon;
uint16_t Tld_real;
uint16_t Tbrd;
uint16_t Itec;
bool TEC_T_Flag;  		// Over-Temperature flag on TEC Driver (OPA569), 0 - temp ok, 1 - overheat happened
bool Stab_ON = 0;			/* Cuvette/Bragg Grating Stabilization Enable signal: 0 - OFF, 1 - ON.
												 Realization of stabilization process in NON-BLOCKING mode */
bool SW_mon; 		  		// Get Optical Switch feedback: pos1 or pos2 
bool SW_set = 0;			// Set state of Optical Switch, pulse = change state
bool LD_CR_PWR_stb = 0; // переключатель стабилизации лазерного диода по току или по выходной мощности с внутр. фотодиода
bool LD_PWR_stb_activated = 0; 			// непрерывное поддержание мощности на внутр.фотодиоде (используется в обработчике от АЦП)
bool LD_PWR_stb_activated_prev = 0; // дополнительная переменная для определения момент фиксации мощности (используется в обработчике от АЦП)

uint16_t FLASH_test_byte; 				// Тестовый байт из FLASH, всегда равен FLASH_TEST_BYTE_INIT. Используется для первичной прошивки FLASH. 	
uint16_t CAN_freq;


/* Service variable */
bool debug_on = 0;

	#ifdef PASSIVE_MODE
	bool passive_mode=0;
	#else
bool passive_mode=1;
	#endif
/*----------------------------------------------------------------------------
  Main function
 *----------------------------------------------------------------------------*/
int main(void)
{
	/**********************************************/
	/* 					Variables initialization				  */
	/**********************************************/
	
	uint16_t tmp[10];
	uint32_t tmp_tick = 0;
	
	huart3 = &var_uart3;														/* Link pointers with variables */
	hadc1 = &var_hadc1;
	hdma1 = &var_hdma1;
	hcan2 = &var_hcan2;
	
	for ( tmp[0]=0; tmp[0]<FLTR_ORDER; tmp[0]++ )		/* Zeros to ExtPD samples array */
	{ExtPD_samples[tmp[0]] = 0; }
	ExtPD_pntr = 0;
	
	
	
	/************************************************/
	/* 					Peripherial initialization				  */
	/************************************************/
	
	HAL_Init();                               /* Initialize the HAL Library     */

  SystemClock_Config();                     /* Configure the System Clock     */
	
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4); // by default
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
	
	HAL_Delay(200);
		
	tmp_tick = HAL_GetTick();			
	HAL_UART_Init(huart3);										/* Initialize UART3 to connect to PC  USED AS A TERMINAL */
		printf("__________________________\r\n");
		printf(" SCFL_Driver_v3.0 \r\n");
		printf(" SystemClock_Config... DONE \r\n");
		printf(" %010d: UART3_Init... DONE \r\n", tmp_tick);
		
	tmp_tick = HAL_GetTick();			
	FLASH_Initialize();
		printf(" %010d: FLASH_init... DONE \r\n", tmp_tick);
		
	
	
	tmp_tick = HAL_GetTick();			
	HAL_CAN_Init(hcan2);										/* Initialize CAN2 - MAIN CONTROL INTERFACE */	
		printf(" %010d: CAN2_init... DONE \r\n", tmp_tick);
	
//	tmp_tick = HAL_GetTick();			
//	DMA_Initialize();										/* Initialize UART3 to connect to PC  USED AS A TERMINAL */
//		#ifdef _UART_CONSOLE_FULL
//		  printf(" %06d: DMA_Init... DONE \n", tmp_tick);
//		#endif
	
	//my_SPI3_Initialize();										/* Initialize SPI3   */
	//printf(" SPI3_Init... DONE \n");
			
	tmp_tick = HAL_GetTick();			
	ALL_PINS_Initialize();											/* Pins for: LEDs, TEC Enable, Switch Enable, Switch Monitor, Buttons  */
		printf(" %010d: ALL_PINS_init... DONE \r\n", tmp_tick);
	
	tmp_tick = HAL_GetTick();			
	DAC_Initialize();													/* Initialize Ditital-to-Anlog Converter */
		printf(" %010d: DAC_init... DONE \r\n", tmp_tick);
		// DAC1 = Laser Diode Power
		// DAC2 = TEC Temperature Setpoint
		// Vdac_ref = 3V3
	
	tmp_tick = HAL_GetTick();			
	ADC_Initialize();													/* Initialize Analog-to-Digital Converter */
		printf(" %010d: ADC_init... DONE \r\n", tmp_tick);
		// ADC1 IN0 = Laser Diode DC Current
		// ADC1 IN1 = Laser Diode RF Current
		// ADC1 IN2 = Laser Diode Inner Photodiode Current
		// ADC1 IN3 = External Photodiode Current 
		// ADC1 IN6 = Laser Diode Temperature ( Tld_real )
		// ADC1 IN7 = Tld-set monitor V-TMP-SET_OUT
		// ADC1 IN8 = Board Temperature
		// ADC1 IN9 = ITEC TEC Current Monitor
	HAL_Delay(500); // wait for all ADC variables first update
	
	tmp_tick = HAL_GetTick();				
	IRQ_Initialize();													/* Initialize Interrupts */
	printf(" %010d: IRQ_init... DONE \r\n", tmp_tick);

					
	/**********************************************/
	/* 						Laser Initial Value  						*/
	/**********************************************/
	
	status = STATE_2;
	
	Condition = 0;
	Condition_Request = STATE_2_INIT;
	tmp_tick = HAL_GetTick();			
	printf(" %010d: state on start up = 0x%02X \r\n", tmp_tick, status);
	
	while (1)
	{ 
/******************************************************/
/*             FOREVER LOOP 													*/
/******************************************************/		
/******************************************************/
/*          FINITE STATE MACHINE                   		*/
/******************************************************/	

	 
		Last_Condition = Condition; 		// сохранение предыдущего состояния
		Condition = Condition_Request;  // запуск обработки нового состояния
		

		switch(Condition)	{

/******************************************************/
/*            STATE_1_LASER_OFF												*/
/******************************************************/	
			
			case STATE_1_LASER_OFF : //все выключено
			
				if (Last_Condition != Condition)	{	
					//	FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %010d: system entered into state %02d \r\n", tmp_tick, Condition);
							// A
					status &= ~FLAG_CVT_IN_USE;
					status &= ~FLAG_FBG_IN_USE;
					status &= ~FLAG_PSV_IN_USE;
					status &= ~FLAG_SCAN_INIT;
					status &= ~FLAG_SCAN;
					status &= ~FLAG_SCAN_FAILED;
					status &= ~FLAG_SCAN_SUCCESS;
					
					if (status & FLAG_LASER_ON) {  
						LD_Off();
						status &= ~FLAG_LASER_ON;
					};
					
					if (status & FLAG_TEC_ON) {
						TEC_Off();
						status &= ~FLAG_TEC_ON;
					};
					
					status &= ~FLAG_SW_ACCESS;
					status &= ~FLAG_PWR_UPDOWN;
					status &= ~FLAG_CAPTURE;
					
							// С
					if ( (status & STATE_1_MASK) != STATE_1 )  {//проверка соответствия флагов текущему состоянию
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %010d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						
					}
				}
				else	{
				
					// NOT FIRST ENTER
					// нет автоматических переходов, только командой
				}			
			break;
			
/******************************************************/
/*            STATE_2_INIT												    */
/******************************************************/						
			
			case STATE_2_INIT: 
			
				if (Last_Condition != Condition) {
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %010d: system entered into state %02d \r\n", tmp_tick, Condition);
					printf(" status= %04X \r\n", status);
													
					// C
					if ( (status & STATE_2_MASK) != STATE_2 )	{ 
						//ОШИБКА СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %010d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						 
					}
					
				}
				else	{
					// NOT FIRST ENTER
					if ( !(syserr & SYSERR_STATE) ) { // переходим в новые состояния только когда с текущее вошли без ошибки SYSERR_STATE
						if (status & FLAG_LASER_ON)	Condition_Request = STATE_3_TEC_START;
          						
					}
				}
			break;
			
/******************************************************/
/*            STATE_3_TEC_START										    */
/******************************************************/			
			
			case STATE_3_TEC_START:
			
				if (Last_Condition != Condition)	{
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %010d: system entered into state %02d \r\n", tmp_tick, Condition);
					printf(" status= %04X \r\n", status);
						// A
					status &= ~FLAG_CVT_IN_USE;
					status &= ~FLAG_FBG_IN_USE;
					status &= ~FLAG_PSV_IN_USE;
					status &= ~FLAG_SCAN_INIT;
					status &= ~FLAG_SCAN;
					status &= ~FLAG_SCAN_FAILED;
					status &= ~FLAG_SCAN_SUCCESS;
					status &= ~FLAG_PWR_UPDOWN;
					status &= ~FLAG_CAPTURE;
				  scan_request=0; // сюда поставить?
					
					status &= ~FLAG_SW_ACCESS; // Сброс старого состояния переключателя, т.к. он далее будет провререн заново.
					Stab_ON = 0; // Сброс переключателя процесса стабилизации (на случай если был включен)
						// B
					if ( !(status & FLAG_TEC_ON) ) {
						TEC_On();
						TEC_Set(Tld_set); 					
						status |= FLAG_TEC_ON;      
					}
						// C		
					if( (status & STATE_3_MASK) != STATE_3)	{
						//ОШИБКА СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %010d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						
					}	
				
				}
				else	{
					// NOT FIRST ENTER
					if ( !(syserr & SYSERR_STATE) ) { // переходим в новые состояния только когда с текущее вошли без ошибки SYSERR_STATE
						if (status & FLAG_LASER_ON)	{
							Condition_Request = STATE_5_PWR_CHANGE;	
							
						}
					}
				}
			break;
			
/******************************************************/
/*            STATE_4_PASSIVE										      */
/******************************************************/			
			
			case STATE_4_PASSIVE:
			
				if (Last_Condition != Condition)	{
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %010d: system entered into state %02d \r\n", tmp_tick, Condition);
						// A
					status &= ~FLAG_CVT_IN_USE;
					status &= ~FLAG_FBG_IN_USE;
					status &= ~FLAG_SCAN_INIT;
					status &= ~FLAG_SCAN;
					status &= ~FLAG_SCAN_FAILED;
					status &= ~FLAG_SCAN_SUCCESS;
					status &= ~FLAG_CAPTURE;
						// B
					status |=FLAG_PSV_IN_USE; 
						// С
					if( (status & STATE_4_MASK) != STATE_4 ) { //проверка соответствия флагов текущему состоянию
						//ОШИБКА СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %010d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						
					}
					 
				}
				else	{
					// NOT FIRST ENTER
					// автоматического перехода из пассивного режима нет, только командой
					
				}					
			break;
			
/******************************************************/
/*            STATE_5_PWR_CHANGE									    */
/******************************************************/			
			
			case STATE_5_PWR_CHANGE:
	    
				if (Last_Condition != Condition)	{
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %010d: system entered into state %02d \r\n", tmp_tick, Condition);
					printf(" status= %04X \r\n", status);
						// A
					status &= ~FLAG_CVT_IN_USE;
					status &= ~FLAG_FBG_IN_USE;
					status &= ~FLAG_PSV_IN_USE;
					status &= ~FLAG_SCAN_INIT;
					status &= ~FLAG_SCAN;
					status &= ~FLAG_SCAN_FAILED;
					status &= ~FLAG_SCAN_SUCCESS;
					// B
					status |= FLAG_PWR_UPDOWN; // сначала выставить, а сброс внутри функции LD_On будет
					
					// С
					if ( (status & STATE_5_MASK) != STATE_5 )	{  //проверка соответствия флагов текущему состоянию
						// ОШИБКА СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %10d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						printf("           status&mask= 0x%04X state= 0x%04X \r\n", status&STATE_5_MASK, STATE_5);
					
					}
					
					LD_On(DC_set_c,DC_set_p);  // перенес сюда так, как проверить надо до сброса FLAG_PWR_UPDOWN, а сброс позже будет в произвольный момент времени. так что так.
					HAL_Delay(1000);
				}
				else	{
					// NOT FIRST ENTER
//				status &= ~FLAG_AUTO_STB;//16.12
//					status |=  FLAG_CVT;//16.12
					if ( !(syserr & SYSERR_STATE) )		{ // переходим в новые состояния только когда в текущее вошли без ошибки SYSERR_STATE
						if( !(status & FLAG_PWR_UPDOWN) && (status & FLAG_TEMP_EQU) )	{
							
						//	if ( (status & FLAG_AUTO_STB) || (status & FLAG_CVT) || (status & FLAG_FBG) ) {
								check_switch();
								if ( !(status & FLAG_SW_ACCESS) ) {
								syserr |= SYSERR_SW_ACCESS;
								printf(" SWITCH->ERROR \r\n");
								}
							//};		
						
							if ( (status & FLAG_SW_ACCESS) && ( (status & FLAG_CVT) || (status & FLAG_AUTO_STB) ) && !(status & FLAG_PWR_UPDOWN ))	 { 
									Condition_Request = STATE_6_CVT_SCAN_INIT;
									printf("           status&flag_cvt= 0x%04X status&flag_auto_stb= 0x%04X \r\n", status & FLAG_CVT, status & FLAG_AUTO_STB);
							}
							else	if ( (status & FLAG_SW_ACCESS) && (status & FLAG_FBG) ) 	{
											Condition_Request = STATE_7_FBG_SCAN_INIT;
									 };
								
							if ( !(status & FLAG_SW_ACCESS) || ( !(status & FLAG_CVT) && !(status & FLAG_FBG) && !(status & FLAG_AUTO_STB) ) )	{
								Condition_Request =STATE_4_PASSIVE;		 		
							};
						}
					}	
				}							
			break;
			
/******************************************************/
/*            STATE_6_CVT_SCAN_INIT								    */
/******************************************************/		
			
			case STATE_6_CVT_SCAN_INIT:// мы выставили флаги начали сканировать а потом в конце поняли что оказывается 
				// набор флагов не тот,а функция уже начала выполняться и к чему это может привести?
			// ничего страшного. просто линия будет найдена, но не захвачена. лазер зависнет в ошибке, находясь на спектральной линии.
		
				if (Last_Condition != Condition)		{
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %10d: system entered into state %02d \r\n", tmp_tick, Condition);
					// A
					status &= ~FLAG_FBG_IN_USE;
					status &= ~FLAG_PSV_IN_USE;
					status &= ~FLAG_SCAN_FAILED;
					status &= ~FLAG_SCAN_SUCCESS;
					status &= ~FLAG_CAPTURE;
					// B
					
					
					//if (lakmus==54) {
//						HAL_Delay(5000);
					switch_cvt();// может быть внести выставление флага use в команду,ведь на кювету я могу и не переключиться, а  флаг выставится
					//}
					status |= FLAG_SCAN_INIT; 					
					// FLAG_SCAN_INIT сбрасывается автоматически в обработчике АЦП, там же где FLAG_TEMP_EQU 
					
					status |= FLAG_SCAN;
					
					status |= FLAG_CVT_IN_USE;  // выставляем именно здесь, т.к. это и есть начало цикла использования кюветы
					// С
					if( (status & STATE_6_MASK) != STATE_6)  { //проверка соответствия флагов текущему состоянию
						//ОШИБКА	СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %10d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						printf(" %10d: status&mask = 0x%04X state = 0x%04X \r\n", tmp_tick, (status & STATE_6_MASK), STATE_6 );
						
					}
					
					// перенес сюда так, как проверить надо до сброса FLAG_SCAN_INIT, а сброс в произвольный момент времени. так что так.
					Tld_set = TEMP_CVT_SCAN_INIT;
					TEC_Set(Tld_set); 
					
							if(tec_work==1){	printf(" tec_work=1 \r\n");}
						while(tec_work==1)
							;
							
							 // задержка для того, чтобы ЦАП успел разнести напряжения на мосту температур и FLAG_TEMP_EQU сбросился
				if(tec_work==0){	printf(" tec_work=0 \r\n");}
				}
				else	{
					// NOT FIRST ENTER
					if ( !(syserr & SYSERR_STATE) )		{ // переходим в новые состояния только когда в текущее вошли без ошибки SYSERR_STATE
						if( (status & FLAG_TEMP_EQU) && (!(status & FLAG_SCAN_INIT)) && (!(status & FLAG_PWR_UPDOWN) ))  {
								
							Condition_Request = STATE_8_CVT_SCAN;
						
						}
					}
				}					
			break;
			
/******************************************************/
/*            STATE_7_FBG_SCAN_INIT								    */
/******************************************************/		
			
			case STATE_7_FBG_SCAN_INIT:
				
				if (Last_Condition != Condition)	{
					// FIRST ENTER 
					tmp_tick = HAL_GetTick();			
					printf(" %10d: system entered into state %02d \r\n", tmp_tick, Condition);
					// A
					status &= ~FLAG_CVT_IN_USE;
					status &= ~FLAG_PSV_IN_USE;
					status &= ~FLAG_SCAN_FAILED;
					status &= ~FLAG_SCAN_SUCCESS;
					status &= ~FLAG_CAPTURE;
					
					// B
//					check_switch();
//					if (lakmus==54) {
						switch_fbg();
//					status |= FLAG_FBG_IN_USE;// что сделать чтобы выставление этого флага было обоснованным?
//					// СЮДА ВСТАВИТЬ ВЫСТАВИТЬ РЕШЕТКУ БРЭГГА
//					}		
					status |= FLAG_SCAN_INIT; 					
					// FLAG_SCAN_INIT сбрасывается автоматически в обработчике АЦП, там же где FLAG_TEMP_EQU 
					
					status |= FLAG_SCAN;
					
					status |= FLAG_FBG_IN_USE;  // 24.11.15
					
					// С
				printf("%0X   status_fbg_command \r\n ", status);// 24.11.15
					if( (status & STATE_7_MASK) != STATE_7)  { //проверка соответствия флагов текущему состоянию
						//ОШИБКА	СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %10d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						
					}
			
					// перенес сюда так, как проверить надо до сброса FLAG_SCAN_INIT, а сброс в произвольный момент времени. так что так.
					Tld_set = TEMP_FBG_SCAN_INIT;
					TEC_Set(Tld_set); 
				
					
				}
				else	{
					// NOT FIRST ENTER
					if ( !(syserr & SYSERR_STATE) )		{ // переходим в новые состояния только когда в текущее вошли без ошибки SYSERR_STATE
						if( (status & FLAG_TEMP_EQU) && !(status & FLAG_SCAN_INIT) )  {
							Condition_Request = STATE_9_FBG_SCAN;
						
						}
					}
				}					
			break;
			
/******************************************************/
/*            STATE_8_CVT_SCAN								        */
/******************************************************/			
			
			case STATE_8_CVT_SCAN:
				
				if (Last_Condition != Condition)	{
				
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %10d: system entered into state %02d \r\n", tmp_tick, Condition);
					// A
					status &= ~FLAG_CAPTURE;
			
//					HAL_Delay(3000);
					ExtPD_outside = ExtPD_mon; // fix current state 
//					status &= FLAG_SCAN_INIT;
					scan_request=33;// поменял

						
					// B

					// С
					if( (status & STATE_8_MASK) != STATE_8)  { //проверка соответствия флагов текущему состоянию
						//ОШИБКА	СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %10d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						printf(" %10d: status&mask = 0x%04X state = 0x%04X \r\n", tmp_tick, (status & STATE_8_MASK), STATE_8 );
						scan_fatality=0;
					}
						//scan();
					
				}
				else	{
					// NOT FIRST ENTER
					if ( !(syserr & SYSERR_STATE) ) { // переходим в новые состояния только когда в текущее вошли без ошибки SYSERR_STATE
						
					//	if( scan_fatality <3) {// проверка сканирования без самого сканирования????
//							scan();// проверка нормальная?
						
							if (status & FLAG_SCAN_SUCCESS) {
								scan_fatality=0;
								Condition_Request = STATE_10_CVT_STAB_WAIT;	
								syserr&= ~SYSERR_CVT_CAPT;							
							}
							else {
								++scan_fatality;
								syserr|=SYSERR_CVT_CAPT;
//								printf("SCAN AGAIN \r\n");
							}
					//	}
						//else 
							//{
							if( !(status & FLAG_AUTO_STB) && (status & FLAG_SCAN_FAILED) )	{
								Condition_Request = STATE_4_PASSIVE;						
							};
							// А где же счетчик неправильных сканирований???
							if ( (status & FLAG_AUTO_STB) && (status & FLAG_SCAN_FAILED) )
								Condition_Request = STATE_7_FBG_SCAN_INIT;						
							// А где же счетчик неправильных сканирований???
						//}
					};
				}				
			break;
			
/******************************************************/
/*            STATE_9_FBG_SCAN								        */
/******************************************************/			

			case STATE_9_FBG_SCAN:
			
				if (Last_Condition != Condition)	{
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %10d: system entered into state %02d \r\n", tmp_tick, Condition);
					// A
					status &= ~FLAG_CAPTURE;
					// B
				//	scan();
//				status &= FLAG_SCAN_INIT;
				scan_request=33;// поменял
					// какие функции запускать?
					ExtPD_outside = ExtPD_mon; // fix current state 
					// С
					printf("%0X   status_fbg_scan_command \r\n ", status);// 24.11.15
					if( (status & STATE_9_MASK) != STATE_9)  { //проверка соответствия флагов текущему состоянию
						//ОШИБКА	СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %10d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						scan_fatality=0;	
					}
				}
				else	{
					// NOT FIRST ENTER
					if ( !(syserr & SYSERR_STATE) )		{ // переходим в новые состояния только когда в текущее вошли без ошибки SYSERR_STATE
//						if( scan_fatality <3) {// проверка сканирования без самого сканирования????
							//scan();						
						
						
						if (status & FLAG_SCAN_SUCCESS) {
							printf("GGG\r\n");
							scan_fatality=0;			
							Condition_Request = STATE_11_FBG_STAB_WAIT;
							syserr&= ~SYSERR_FBG_CAPT;			
						}
//         else {
//					  ++scan_fatality;
//			      syserr|=SYSERR_CVT_CAPT;
//						printf("SCAN AGAIN \r\n");
//					 
//				 }	
//			 }				
//         else {			 
						if (status & FLAG_SCAN_FAILED)	{
							Condition_Request = STATE_4_PASSIVE;						
						}
 
						//}
					};
				}			
			break;
		 					
/******************************************************/
/*            STATE_10_CVT_STAB_WAIT	  			        */
/******************************************************/			
			
			case STATE_10_CVT_STAB_WAIT:
				
				if (Last_Condition != Condition)		{
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %10d: system entered into state %02d \r\n", tmp_tick, Condition);
					// B
					// запускаем не блокирующую стабилизацию, ждём пока прерывание от АЦП выставит FLAG_CAPTURE
					Stab_ON = 0x01;
					// С
					if( (status & STATE_10_MASK) != STATE_10)  { //проверка соответствия флагов текущему состоянию
						//ОШИБКА	СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						Stab_ON = 0x00;
						syserr |= SYSERR_STATE;
						printf(" %10d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
					}
				}
				else 	{
					// NOT FIRST ENTER
					if ( !(syserr & SYSERR_STATE) )		{ // переходим в новые состояния только когда в текущее вошли без ошибки SYSERR_STATE
				
						if (status & FLAG_CAPTURE) {
							Condition_Request = STATE_12_CVT_STAB_WORK;
						}
						
						if( !(status & FLAG_AUTO_STB) && (status & FLAG_SCAN_FAILED) )	{
							Condition_Request = STATE_4_PASSIVE;
						}
						
						// А где же счетчик неправильных сканирований???
						
						if( (status & FLAG_AUTO_STB) && (status & FLAG_SCAN_FAILED) )	{
							Condition_Request = STATE_7_FBG_SCAN_INIT;
						}
						
						// А где же счетчик неправильных сканирований???
						
					}
				}
			break;
		
/******************************************************/
/*            STATE_11_FBG_STAB_WAIT	  			        */
/******************************************************/			
			
			case STATE_11_FBG_STAB_WAIT:
				
				if ( Last_Condition != Condition )	{
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %10d: system entered into state %02d \r\n", tmp_tick, Condition);
					// B
					// запускаем не блокирующую стабилизацию, ждём пока прерывание от АЦП выставит FLAG_CAPTURE
					Stab_ON = ENABLE;
					// С
					if( (status & STATE_11_MASK) != STATE_11)  { //проверка соответствия флагов текущему состоянию
						//ОШИБКА	СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %10d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
							
					}
				}
				else	{
					// NOT FIRST ENTER
					if ( !(syserr & SYSERR_STATE) )		{ // переходим в новые состояния только когда в текущее вошли без ошибки SYSERR_STATE
						
						if (status & FLAG_CAPTURE) {
							Condition_Request = STATE_13_FBG_STAB_WORK;
							
						}
						
						if (status & FLAG_SCAN_FAILED) {
							Condition_Request = STATE_4_PASSIVE;
						
						}
						// А где же счетчик неправильных сканирований???
					}
				}
			break;

/******************************************************/
/*            STATE_12_CVT_STAB_WORK	  			        */
/******************************************************/			
			
			case STATE_12_CVT_STAB_WORK:
				
				if ( Last_Condition != Condition )	{
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %10d: system entered into state %02d \r\n", tmp_tick, Condition);
					// A
					status &= ~FLAG_SCAN;
			     // B
					// какие функции запускать?
					// С
					if( (status & STATE_12_MASK) != STATE_12)  { //проверка соответствия флагов текущему состоянию
						//ОШИБКА	СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %10d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						printf(" %10d: status&mask = 0x%04X state = 0x%04X \r\n", tmp_tick, (status & STATE_12_MASK), STATE_12 );					
					}
				}
				else	{
							
					// NOT FIRST ENTER
					//что делать при Н заходе?						
					// ничего - просто получать удовльствие что система дошла-таки до нужного состояния!)					
				}
			break;			 

/******************************************************/
/*            STATE_13_FBG_STAB_WORK	  			        */
/******************************************************/			
			
			case STATE_13_FBG_STAB_WORK:
				
				if (Last_Condition != Condition)	{
					// FIRST ENTER
					tmp_tick = HAL_GetTick();			
					printf(" %10d: system entered into state %02d \r\n", tmp_tick, Condition);
					// A
					status &= ~FLAG_SCAN;
			    // B
					// какие функции запускать?
					// С
					if( (status & STATE_13_MASK) != STATE_13)  { //проверка соответствия флагов текущему состоянию
						//ОШИБКА	СОСТОЯНИЯ
						tmp_tick = HAL_GetTick();			
						syserr |= SYSERR_STATE;
						printf(" %10d: SYSERR_STATE happened in %02d state \r\n", tmp_tick, Condition);
						
					}
				}
				else	{
					 
					// NOT FIRST ENTER
					//что делать при Н заходе?	
					// ничего - просто получать удовльствие что система дошла-таки до нужного состояния!)
				}
			break;
			}  // end of SWITCH
		
		} // end of WHILE(1)
/******************************************************/
/*             END OF FOREVER LOOP										*/
/******************************************************/
		
} // end of MAIN	


/**********************************/
/*     COMMAND INTERPRETER        */
/**********************************/
/**
  * @brief  Function interpreters commands to laser from different interfaces
  * @param  Global variable "cmd"
  * @retval One of CMD_ErrorTypeDef (if Success - CMD_ERROR_NONE = 0x00) 
  */
int32_t cmd_interp(void) {
	
	uint8_t err;
	uint16_t temp[10];
	//uint16_t gm;
	//uint32_t  Error;
	
	err = CMD_ERROR_NONE;
	
	if (cmd.State == CMD_STATE_BUSY) {  
		// input Condition means that one if interfaces catched command
		// + verification that each received command launched only once
		
		switch (cmd.CmdCode) {
			
		case 'A':   // CMD-TEC-ON: Peltier ON
			
			if ( !(status & FLAG_TEC_ON) ) {   
			// A		
						TEC_On(); 					
						TEC_Set(Tld_set); 	      
	          status |= FLAG_TEC_ON; 
			// B		
			printf("CMD-TEC-ON DONE \r\n");
			err = CMD_ERROR_NONE;  // Success on Peltier ON
			// C						 
		  Condition_Request = STATE_3_TEC_START;	
			}				
		break;
		
		case 'B':   // CMD-TEC-OFF: Peltier OFF
			
			if (status & FLAG_TEC_ON)	{
				LD_Off();
				status &= ~FLAG_LASER_ON;
				TEC_Off();
				status &= ~FLAG_TEC_ON;
		  //  B
				printf("CMD-TEC-OFF DONE \r\n"); 
				err = CMD_ERROR_NONE;  // Success on Peltier OFF
		  // C
				Condition_Request = STATE_1_LASER_OFF ;	
			}
		break;
		
		case 'C':   // CMD-LASER-ON: Smooth rising LD current to setpoint
			
			if  ( (!(status & FLAG_LASER_ON))&&(status & FLAG_TEC_ON) ) 	{
			 // A				
			  LD_On(DC_set_c,DC_set_p);
				// ПРОВЕРКА БЛОКИРУЕТ ВКЛЮЧЕНИЕ ЛАЗЕРА ЕСЛИ БЫЛ ПЕРЕГРЕВ ИЛИ СБОЙ В ПЕЛЬТЬЕ 
				// для продолжения работы надо либо перезагрузить устройство, либо сбросить регистр SYSERR
			  if ( !(syserr & SYSERR_TEC_OVRC) && !(syserr & SYSERR_LAS_OVRT) ) { 
					status |= FLAG_LASER_ON; 					
				}
		   //  B
				printf("CMD-LASER-ON DONE \r\n");
				err = CMD_ERROR_NONE;  // Success on Smooth LD start up
			 // C		
				Condition_Request = STATE_3_TEC_START ;	
			}
		break;
		
		case 'D':   // CMD-LASER-OFF: Smooth put off LD current
			if ( status & FLAG_LASER_ON)	{
				// A	
				LD_Off();
				status &= ~FLAG_LASER_ON;
				// B
				printf("CMD-LASER-OFF DONE \r\n");
				err = CMD_ERROR_NONE;  // Success on Switching LD off
				// C
				Condition_Request = STATE_3_TEC_START ;		
			}
		break;
		
		case 'E':   // CMD-DC-SET: 
			temp[0] = ( (cmd.Byte2<<4) | ( 0x0F & cmd.Byte3) );
			if ( (temp[0] > DC_set_min) && (temp[0] < DC_set_max) )
			{ DC_set_c = temp[0];
			 if( status & FLAG_LASER_ON)
			 {
				 // A
				status &= ~FLAG_CVT_IN_USE;
				status &= ~FLAG_FBG_IN_USE;
				 // B
				printf(" 1 byte= 0x%02X, 2 byte= 0x%02X  DC_set = 0d%04d \r\n  ",cmd.Byte2,cmd.Byte3,DC_set_c);
				err = CMD_ERROR_NONE; // Success on setting new DC
				 // C
				 Condition_Request = STATE_3_TEC_START ;	
			 }
		 }
			else {
				printf(" DC_set_c=0d%04d is too high (max=0d%04d)\r\n ", temp[0], DC_set_max);
				err = CMD_ERROR_OOR;
			};
			printf("CMD-DC-SET DONE \r\n");
		break;
		
		case 'F':   // CMD-STAB-CVT:  
				if(passive_mode) {	
			if (cmd.Byte2 == 0x01) {
				if ( !(status&FLAG_CVT) && (status&FLAG_SW_ACCESS) ) { // ON
					status |= FLAG_CVT;
					if ( !(status&FLAG_AUTO_STB))	{
						printf(" CVT Stab ACTIVATED  \r\n");
						Condition_Request = STATE_3_TEC_START ;
						err = CMD_ERROR_NONE;						
					}	
				}
		  }
		else	if ( (status&FLAG_CVT) && (status&FLAG_SW_ACCESS) ) { // OFF
						status &= ~FLAG_CVT;
						printf(" CVT Stab DeACTIVATED \r\n");
						if ( !(status&FLAG_AUTO_STB) && (status&FLAG_FBG) )	{
							Condition_Request = STATE_3_TEC_START ;	
							printf("CMD-STAB-CVT DONE \n");						
							err = CMD_ERROR_NONE;  // Success on CMD-STAB-CVT
						}
						else	{
							Condition_Request = STATE_3_TEC_START ;
							printf("CMD-STAB-CVT DONE \r\n");
							err = CMD_ERROR_NONE;  // Success on CMD-STAB-CVT
						}
					}
				}
				else { 
				printf("Command CMD-STAB-CVT blocked in passive mode \r\n"); 
				err = CMD_ERROR_BLOCKED;
					}
		break;
		
		case 'G':   // CMD-STAB-FBG:
   
			if(passive_mode) {	
				if (cmd.Byte2 == 0x01) {
			if ( !(status&FLAG_FBG)  ) // ON
				{
					printf("%0X status_fbg_command \r\n ", status);
					status |= FLAG_FBG;
					printf(" FBG Stab ACTIVATED \r\n");							
				if ( (!(status&FLAG_AUTO_STB))&&(!(status&FLAG_CVT)))
				{
				Condition_Request = STATE_3_TEC_START ;
					err = CMD_ERROR_NONE;
				}
			    }
		        }
			else if ( (status&FLAG_FBG)&&(status&FLAG_SW_ACCESS))
			 {
				  printf("%0X status_fbg_0 command \r\n ", status);
			 if ( status&FLAG_FBG) 
				 {
						status &= ~FLAG_FBG;
					 printf("%0X status_fbg_1 command \r\n ", status);
						printf(" FBG Stab DeACTIVATED \r\n");
					 	
					 }
			 if ( (status&FLAG_AUTO_STB)&&(status&FLAG_CVT))
			 {
				  printf("%0X status_fbg_2 command \r\n ", status);
			 Condition_Request = STATE_3_TEC_START ;
				 err = CMD_ERROR_NONE;
			 } 	 
				 }
			 else if(!(status&FLAG_SW_ACCESS) )
			 {
				 err = CMD_ERROR_OOR;
				 syserr |= SYSERR_SW_ACCESS;
			 }
			  printf("%0X status_fbg_3 command \r\n ", status);
			printf("CMD-STAB-FBG DONE \r\n");
		
			  Condition_Request = STATE_3_TEC_START ;// 24.11.15
			err = CMD_ERROR_NONE;  // Success on CMD-STAB-FBG
		 }
					else { 
				printf("Command CMD-STAB-AUTO blocked in passive mode \r\n"); 
				err = CMD_ERROR_BLOCKED;
					}
		break;
		
		case 'H':   // CMD-STAB-AUTO:
			if(passive_mode) {		
			if (cmd.Byte2 == 0x01) {
				if ( !(status&FLAG_AUTO_STB)&&(status&FLAG_SW_ACCESS))	{
					if ( !(status&FLAG_AUTO_STB) ) {
						status |= FLAG_AUTO_STB;
						Condition_Request = STATE_3_TEC_START ; 
						printf(" AUTO Stab ACTIVATED \r\n");					
					}
				}
		  }
			else if ( (status & FLAG_AUTO_STB) && (status & FLAG_SW_ACCESS)) {
							
										Condition_Request = STATE_3_TEC_START;
										printf(" AUTO Stab DeACTIVATED \r\n");
										status &= ~FLAG_AUTO_STB;		
                   				
								 
					 }

				printf("%0X status_autostab command \r\n ", status);
				printf("CMD-STAB-AUTO DONE \r\n");
				err = CMD_ERROR_NONE;  // Success on CMD-STAB-AUTO
				 }
			else { 
				printf("Command CMD-STAB-FBG blocked in passive mode \r\n"); 
				err = CMD_ERROR_BLOCKED;
			}
		break;
		
		case 'I':   // CMD-STATUS
			
			// формирование пакета происходит непосредственно перед отправкой
			// т.е. сразу после данного оператора SWITCH
		
			err = CMD_ERROR_NONE;  // Success on CMD-STATUS
		
		break;
		
		case 'J':   // CMD-TEMP-SET  
			err = CMD_ERROR_NONE; // Success on CMD-TEMP-SET
			if (debug_on) {
				temp[0] = ( (cmd.Byte2<<4) | ( 0x0F & cmd.Byte3) );
				if ( (temp[0] < Tld_set_max) && (temp[0] > Tld_set_min) ) {
					Tld_set = temp[0];
					TEC_Set(Tld_set);
					printf(" 1 byte= 0d%02d, 2 byte= 0d%02d  Tld_set = 0d%04d \r\n  ",cmd.Byte2,cmd.Byte3,Tld_set);
				}
				else {
					printf(" Tld_set=0d%04d out of range (min=0d%04d, max=0d%04d)\r\n ", temp[0], Tld_set_min, Tld_set_max);
				};
				printf("CMD-TEMP-SET DONE \r\n");			
			}
			else { 
				printf("Command CMD-TEMP-SET blocked in user mode \r\n"); 
				err = CMD_ERROR_BLOCKED;
			}
		break;
			
		case 'K':   // CMD-TEMP-SET-FIX  // ДА ОТКУДА ОНИ БЕРУТСЯ , ЭТИ ЗАГАДОЧНЫЕ КОМАНДЫ?
			err = CMD_ERROR_NONE;  // Success on CMD-TEMP-SET-FIX
			if (debug_on) {
				// LOAD NEW TEMP-INIT VALUE IN FLASH
				HAL_FLASH_Unlock();
				HAL_Delay(FLASH_delay_l);
				temp[1] = *((uint32_t *)FLASH_DC_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[2] = *((uint32_t *)FLASH_DC_MAX_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[3] = *((uint32_t *)FLASH_DC_MIN_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[4] = *((uint32_t *)FLASH_TEMP_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[5] = *((uint32_t *)FLASH_CAN_FREQ_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[6] = *((uint32_t *)FLASH_TEST_BYTE_ADR);
				HAL_Delay(FLASH_delay_s);
			
				temp[4] = Tld_set;
				
				HAL_FLASHEx_Erase(&Flash,&Sector_Error);	
				HAL_Delay(FLASH_delay_l);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_INIT_ADR, temp[1]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MAX_ADR, temp[2]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MIN_ADR, temp[3]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEMP_INIT_ADR, temp[4]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_CAN_FREQ_ADR, temp[5]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEST_BYTE_ADR, temp[6]);
				HAL_Delay(FLASH_delay_s);
				
				HAL_FLASH_Lock();
				// End of LOAD NEW TEMP_CENTER VALUE IN FLASH
				
				printf("	FLASH REburning for NEW TEMP_INIT... DONE \r\n");		
				
				err = CMD_ERROR_NONE; // Success on setting new DC
				
				printf("New Tld value saved in flash as default \r\n");
				printf("CMD-TEMP-SET-FIX DONE \r\n");
			}
			else { 
				printf("Command CMD-TEMP-SET-FIX blocked in user mode \r\n"); 
				err = CMD_ERROR_BLOCKED;
			}
		break;
			
		case 'L':   // CMD-DEBUG  // Я ТАК ПОНИМАЮ,ЭТО ТЕ САМЫЕ ДЕБАГОВЫЕ ЗАГАДОЧНЫЕ КОМАНДЫ ШРЁДИНГЕРА
			if (cmd.Byte2 == 0) {
					debug_on = 0; 
					printf("DEBUG MODE is OFF \r\n");
				};
			if (cmd.Byte2 == 1) {
					debug_on = 1; 
					printf("DEBUG MODE is ON \r\n");
				};
			if (cmd.Byte2 > 1) {
					printf("2nd byte values: 0 - OFF, 1 - ON \r\n"); 
				};
				err = CMD_ERROR_NONE;  // Success on CMD-DEBUG
		break;
		
		case 'M': // CMD-DC-FIX
			temp[0] = ( (cmd.Byte2<<4) | ( 0x0F & cmd.Byte3) );
			if ( (temp[0] > DC_set_min) && (temp[0] < DC_set_max) )	{ 
				
				// LOAD NEW DC_set_c VALUE IN FLASH
				HAL_FLASH_Unlock();
				HAL_Delay(FLASH_delay_l);
				temp[1] = *((uint32_t *)FLASH_DC_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[2] = *((uint32_t *)FLASH_DC_MAX_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[3] = *((uint32_t *)FLASH_DC_MIN_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[4] = *((uint32_t *)FLASH_TEMP_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[5] = *((uint32_t *)FLASH_CAN_FREQ_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[6] = *((uint32_t *)FLASH_TEST_BYTE_ADR);
				HAL_Delay(FLASH_delay_s);
			
				temp[1] = temp[0];
				
				HAL_FLASHEx_Erase(&Flash,&Sector_Error);	
				HAL_Delay(FLASH_delay_l);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_INIT_ADR, temp[1]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MAX_ADR, temp[2]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MIN_ADR, temp[3]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEMP_INIT_ADR, temp[4]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_CAN_FREQ_ADR, temp[5]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEST_BYTE_ADR, temp[6]);
				HAL_Delay(FLASH_delay_s);
				
				HAL_FLASH_Lock();
				// End of LOAD NEW DC_set_c VALUE IN FLASH
				
				printf("	FLASH REburning for NEW DC_set_c... DONE \r\n");		
				printf(" 1 byte= 0x%02X, 2 byte= 0x%02X  DC_set = 0d%04d \r\n  ",cmd.Byte2,cmd.Byte3,DC_set_c);
				err = CMD_ERROR_NONE; // Success on setting new DC
			}
			else {
				printf(" DC_set_c=0d%04d is too high (max=0d%04d)\r\n ", temp[0], DC_set_max);
				err = CMD_ERROR_OOR;
			};
			printf("CMD-DC-FIX DONE \r\n");
			
			err = CMD_ERROR_NONE;
		break;
		
	  case 'N': // CMD-DC-MAX-FIX
			temp[0] = ( (cmd.Byte2<<4) | ( 0x0F & cmd.Byte3) );
			if ( (temp[0] > 0) && (temp[0] <= 4095) )	{ 
				
				// LOAD NEW DC_set_max VALUE IN FLASH
				HAL_FLASH_Unlock();
				HAL_Delay(FLASH_delay_l);
				temp[1] = *((uint32_t *)FLASH_DC_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[2] = *((uint32_t *)FLASH_DC_MAX_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[3] = *((uint32_t *)FLASH_DC_MIN_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[4] = *((uint32_t *)FLASH_TEMP_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[5] = *((uint32_t *)FLASH_CAN_FREQ_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[6] = *((uint32_t *)FLASH_TEST_BYTE_ADR);
				HAL_Delay(FLASH_delay_s);
			
				temp[2] = temp[0];
				
				HAL_FLASHEx_Erase(&Flash,&Sector_Error);	
				HAL_Delay(FLASH_delay_l);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_INIT_ADR, temp[1]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MAX_ADR, temp[2]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MIN_ADR, temp[3]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEMP_INIT_ADR, temp[4]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_CAN_FREQ_ADR, temp[5]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEST_BYTE_ADR, temp[6]);
				HAL_Delay(FLASH_delay_s);
				
				HAL_FLASH_Lock();
				// End of LOAD NEW DC_set_max VALUE IN FLASH
				
				printf("	FLASH REburning for NEW DC_set_max... DONE \r\n");		
				printf(" 1 byte= 0x%02X, 2 byte= 0x%02X  DC_set_max = 0d%04d \r\n  ",cmd.Byte2,cmd.Byte3,temp[0]);
				err = CMD_ERROR_NONE; // Success on setting new DC
			}
			else {
				printf(" DC_set_max=0d%04d is too high (max=0d%04d)\r\n ", temp[0], 4095);
				err = CMD_ERROR_OOR;
			};
			printf("CMD-DC-MAX-FIX DONE \r\n");
			
			err = CMD_ERROR_NONE;
		break;
			
	  case 'O': // CMD-DC-MIN-FIX
			temp[0] = ( (cmd.Byte2<<4) | ( 0x0F & cmd.Byte3) );
			if (temp[0] < 4095)	{ 
				
				// LOAD NEW DC_set_min VALUE IN FLASH
				HAL_FLASH_Unlock();
				HAL_Delay(FLASH_delay_l);
				temp[1] = *((uint32_t *)FLASH_DC_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[2] = *((uint32_t *)FLASH_DC_MAX_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[3] = *((uint32_t *)FLASH_DC_MIN_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[4] = *((uint32_t *)FLASH_TEMP_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[5] = *((uint32_t *)FLASH_CAN_FREQ_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[6] = *((uint32_t *)FLASH_TEST_BYTE_ADR);
				HAL_Delay(FLASH_delay_s);
			
				temp[3] = temp[0];
				
				HAL_FLASHEx_Erase(&Flash,&Sector_Error);	
				HAL_Delay(FLASH_delay_l);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_INIT_ADR, temp[1]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MAX_ADR, temp[2]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MIN_ADR, temp[3]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEMP_INIT_ADR, temp[4]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_CAN_FREQ_ADR, temp[5]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEST_BYTE_ADR, temp[6]);
				HAL_Delay(FLASH_delay_s);
				
				HAL_FLASH_Lock();
				// End of LOAD NEW DC_set_min VALUE IN FLASH
				
				printf("	FLASH REburning for NEW DC_set_min... DONE \r\n");		
				printf(" 1 byte= 0x%02X, 2 byte= 0x%02X  DC_set_min = 0d%04d \r\n  ",cmd.Byte2,cmd.Byte3,temp[0]);
				err = CMD_ERROR_NONE; // Success on setting new DC
			}
			else {
				printf(" DC_set_min=0d%04d is too high (min=0d%04d)\r\n ", temp[0], 0);
				err = CMD_ERROR_OOR;
			};
			printf("CMD-DC-MIN-FIX DONE \r\n");
			
			err = CMD_ERROR_NONE;
		break;
				
		case 'P': //CMD-STAB-START
			Condition_Request = STATE_3_TEC_START ;		
			printf(" Scan Reset \r\n");
			err = CMD_ERROR_NONE;
			printf("CMD-STAB-START DONE \r\n");
		break;
		
		case 'Q': //CMD-CAN-FREQ
			
			err = CMD_ERROR_NONE;
			switch (cmd.Byte2) {
				case 0:
					temp[0] = 125;
					printf(" CAN freq = 125 kHz will be active after reset \r\n");
				break;
				case 1:
					temp[0] = 250;
					printf(" CAN freq = 250 kHz will be active after reset \r\n");
				break;
				case 2:
					temp[0] = 500;
					printf(" CAN freq = 500 kHz will be active after reset \r\n");
				break;
				case 3:
					temp[0] = 1000;
					printf(" CAN freq = 1000 kHz will be active after reset \r\n");
				break;
				default:	
					printf(" Wrong value, possible values - 0,1,2,3 \r\n");
					err = CMD_ERROR_OOR;
				break;
			};
			if (err != CMD_ERROR_OOR) {
				// LOAD NEW CAN_freq VALUE IN FLASH
				HAL_FLASH_Unlock();
				HAL_Delay(FLASH_delay_l);
				temp[1] = *((uint32_t *)FLASH_DC_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[2] = *((uint32_t *)FLASH_DC_MAX_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[3] = *((uint32_t *)FLASH_DC_MIN_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[4] = *((uint32_t *)FLASH_TEMP_INIT_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[5] = *((uint32_t *)FLASH_CAN_FREQ_ADR);
				HAL_Delay(FLASH_delay_s);
				temp[6] = *((uint32_t *)FLASH_TEST_BYTE_ADR);
				HAL_Delay(FLASH_delay_s);
			
				temp[5] = temp[0];
				
				HAL_FLASHEx_Erase(&Flash,&Sector_Error);	
				HAL_Delay(FLASH_delay_l);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_INIT_ADR, temp[1]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MAX_ADR, temp[2]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_DC_MIN_ADR, temp[3]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEMP_INIT_ADR, temp[4]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_CAN_FREQ_ADR, temp[5]);
				HAL_Delay(FLASH_delay_s);
				HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,FLASH_TEST_BYTE_ADR, temp[6]);
				HAL_Delay(FLASH_delay_s);
				
				HAL_FLASH_Lock();
				// End of LOAD NEW CAN_freq VALUE IN FLASH
				
				printf("	FLASH REburning for NEW CAN_freq... DONE \r\n");		
				printf(" 1 byte= 0x%02X, 2 byte= 0x%02X  \r\n",cmd.Byte2,cmd.Byte3);
			}
						  
			printf("CMD-CAN-FREQ DONE\r\n");
		break;
			
		case 'R': // CMD-ERR-RESET
			syserr = 0;
			printf(" Error register  cleaned\r\n");
			err = CMD_ERROR_NONE;  
		break;
		
		case 'S':   // CMD-SWITCH-MANUAL // Ручное управление свичом byte2 = 1 - кювета byte2 = 2 - решетка
			if (cmd.Byte2 == 1) {
					switch_cvt();
				}
			else if (cmd.Byte2 == 2) {
						switch_fbg();
				}
			else {
				printf(" Spec ref № %02d is absent (only 1 and 2 exist)\r\n", cmd.Byte2);
				err = CMD_ERROR_OOR;
			}
			err = CMD_ERROR_NONE;  // Success on CMD-SWITCH-MANUAL
		break;
		
		case 'T':   // CMD-PWR-STB	// Включение/выключение стабилизации по мощности (если выкл, то работает стабилизация по току)
			if (cmd.Byte2 == 0) {
					LD_CR_PWR_stb = 0; 
					LD_PWR_stb_activated = 0; 
				}
			else if (cmd.Byte2 == 1) {
						LD_CR_PWR_stb = 1; 
						LD_PWR_stb_activated = 1; 
				}
			else {
				printf(" Values 0 and 1 only are in use)\r\n ");
				err = CMD_ERROR_OOR;
			}
			err = CMD_ERROR_NONE;  // Success on CMD-PWR-STB
		break;
			
		case 'U': //CMD-FLASH-RESET
			printf(" FLASH RESET ... ");
			HAL_FLASH_Unlock();
			HAL_Delay(FLASH_delay_l);			
			HAL_FLASHEx_Erase(&Flash,&Sector_Error);	
			HAL_Delay(FLASH_delay_l);			
			HAL_FLASH_Lock();
			HAL_Delay(FLASH_delay_s);
			printf(" ... DONE, please reset system\r\n");
			err = CMD_ERROR_NONE;
		break;
		
		
		case 'Z':   // CMD-EXTPD-STATUS 
			printf("%04d,%04d\n", ExtPD_mon,ExtPD_avg);
			err = CMD_ERROR_NONE;  // Success on CMD-STATUS
		break;
				
		default:
			printf("Incorrect command code: nothing was done\r\n");
			err = CMD_ERROR_NOCMD;
		break;
				} // END OF SWITCH OPERATOR
		
		if (cmd.CmdCode != 'I') { 
				// формируем ответный пакет для обычных команд
				hcan2->pTxMsg = &CAN2_Tx;
				CAN2_Tx.DLC = 2;
				CAN2_Tx.Data[0] = cmd.CmdCode;
				CAN2_Tx.Data[1] = err;
				
				// отправляем пакет по назначению
				if (cmd.Interface == CMD_INT_UART) {
					printf(" cmd.Code = %s %02X err = %02d \r\n",&cmd.CmdCode,CAN2_Tx.Data[0],CAN2_Tx.Data[1]);	
				}
				if (cmd.Interface == CMD_INT_CAN) {
					HAL_CAN_Transmit_IT(hcan2);
					printf(" cmd.Code = %s %02X err = %02d \r\n",&cmd.CmdCode,CAN2_Tx.Data[0],CAN2_Tx.Data[1]);	
					printf(" CAN COPY: %02X %02X \r\n",CAN2_Tx.Data[0],CAN2_Tx.Data[1]);					
				}			
			}
			else {
				// Формируем ответ для команды "I" - CMD-STATUS
				if ( (cmd.Byte2 != 0x01) && (cmd.Byte2 != 0x02) && (cmd.Byte2 != 0x03) ) { // ошибка, CAN-сообщения с таким номером нет - поэтому ошибка CMD-ERROR-OOR
					err = CMD_ERROR_OOR;
					// формируем ответный пакет
					hcan2->pTxMsg = &CAN2_Tx;
					CAN2_Tx.DLC = 2;
					CAN2_Tx.Data[0] = cmd.CmdCode;
					CAN2_Tx.Data[1] = err;
				
					// отправляем пакет по назначению
					if (cmd.Interface == CMD_INT_UART) {
						printf(" CAN message with %02d number is absent (exist 1,2 and 3) \r\n",cmd.Byte2);	
						printf(" cmd.Code = %s %02X err = %02d \r\n",&cmd.CmdCode,CAN2_Tx.Data[0],CAN2_Tx.Data[1]);	
					}
					if (cmd.Interface == CMD_INT_CAN) {
						HAL_CAN_Transmit_IT(hcan2);
						printf(" CAN message with %02d number is absent (exist 1,2 and 3) \r\n",cmd.Byte2);	
						printf(" cmd.Code = %s %02X err = %02d \r\n",&cmd.CmdCode,CAN2_Tx.Data[0],CAN2_Tx.Data[1]);	
						printf(" CAN COPY: %02X %02X \r\n",CAN2_Tx.Data[0],CAN2_Tx.Data[1]);					
					}
				} 
				else {
					
					switch (cmd.Byte2) {
						case 0x01: 
							// Формируем CAN Сообщение 1 для команды "I" - CMD-STATUS
							hcan2->pTxMsg = &CAN2_Tx;
							CAN2_Tx.DLC = 8;
							CAN2_Tx.Data[0] = cmd.CmdCode;
							CAN2_Tx.Data[1] = module_serial;
							CAN2_Tx.Data[2] = driver_serial;
							CAN2_Tx.Data[3] = software_version;
							CAN2_Tx.Data[4] = (uint8_t) (0x00FF & InnPD_mon);
							CAN2_Tx.Data[5] = (uint8_t) (InnPD_mon >> 8);
							CAN2_Tx.Data[6] = (uint8_t) (0x00FF & ExtPD_mon);
							CAN2_Tx.Data[7] = (uint8_t) (ExtPD_mon >> 8);
							printf(" cmd.Code = %s %02X err = %02d \r\n",&cmd.CmdCode,CAN2_Tx.Data[0],CAN2_Tx.Data[1]);	
							printf(" cmd.Code = %02X module_serial = %02X driver_serial = %02X software_version = %02X \r\n",CAN2_Tx.Data[0],CAN2_Tx.Data[1],CAN2_Tx.Data[2],CAN2_Tx.Data[3]);
						  printf(" InnPD_mon_L = %02X InnPD_mon_H = %02X ExtPD_mon_L = %02X ExtPD_mon_H = %02X \r\n",CAN2_Tx.Data[4],CAN2_Tx.Data[5],CAN2_Tx.Data[6],CAN2_Tx.Data[7]);
							printf("ExtPD = 0d%04d \r\n",ExtPD_mon);
							printf("Tld_real = 0d%04d \r\n",Tld_real);
							break;
						case 0x02: 
						// Формируем CAN Сообщение 2 для команды "I" - CMD-STATUS
							CAN2_Tx.DLC = 8;
							CAN2_Tx.Data[0] = (uint8_t) (0x00FF & Tld_real);
							CAN2_Tx.Data[1] = (uint8_t) (Tld_real >> 8);
							CAN2_Tx.Data[2] = (uint8_t) (0x00FF & Tbrd);
							CAN2_Tx.Data[3] = (uint8_t) (Tbrd >> 8);
							CAN2_Tx.Data[4] = (uint8_t) (0x00FF & status);
							CAN2_Tx.Data[5] = (uint8_t) (status >> 8); 				
							CAN2_Tx.Data[6] = syserr;
							CAN2_Tx.Data[7] = 0xAA;  // тестовый байт, всегда передаёт константу
							printf(" cmd.Code = %s %02X err = %02d \r\n",&cmd.CmdCode,CAN2_Tx.Data[0],CAN2_Tx.Data[1]);	
							printf(" Tld_real_L = %02X Tld_real_H = %02X Tbrd_L = %02X Tbrd_H = %02X \r\n",CAN2_Tx.Data[0],CAN2_Tx.Data[1],CAN2_Tx.Data[2],CAN2_Tx.Data[3]);
							printf(" status_L = %02X status_H = %02X syserr = %02X test_0xAA = %02X \r\n",CAN2_Tx.Data[4],CAN2_Tx.Data[5],CAN2_Tx.Data[6],CAN2_Tx.Data[7]);
							break;
						case 0x03: 
						// Формируем CAN Сообщение 3 для команды "I" - CMD-STATUS
							CAN2_Tx.DLC = 8;
							CAN2_Tx.Data[0] = (uint8_t) (0x00FF & DC_mon);
							CAN2_Tx.Data[1] = (uint8_t) (DC_mon >> 8);
							CAN2_Tx.Data[2] = (uint8_t) Condition;
							CAN2_Tx.Data[3] = (uint8_t) Last_Condition;
							CAN2_Tx.Data[4] = (uint8_t) (0x00FF & Itec);
							CAN2_Tx.Data[5] = (uint8_t) (Itec >> 8);
							CAN2_Tx.Data[6] = (uint8_t) (0x00FF & Tld_setmon);
							CAN2_Tx.Data[7] = (uint8_t) (Tld_setmon >> 8);
							printf(" cmd.Code = %s %02X err = %02d \r\n",&cmd.CmdCode,CAN2_Tx.Data[0],CAN2_Tx.Data[1]);	
							printf(" DC_mon_L = %02X DC_mon_H = %02X Condition = %02X Last_Condition = %02X \r\n",CAN2_Tx.Data[0],CAN2_Tx.Data[1],CAN2_Tx.Data[2],CAN2_Tx.Data[3]);
							printf(" Itec_L = %02X Itec_H = %02X Tld_setmon_L = %02X Tld_setmon_H = %02X \r\n",CAN2_Tx.Data[4],CAN2_Tx.Data[5],CAN2_Tx.Data[6],CAN2_Tx.Data[7]);
							break;
						default:
							// по умолчанию выдает ошибку, что сообщения с номером нету. ошибка CMD-ERR-OOR
							break;
					} // end of switch (cmd.Byte2)
						// отправляем пакет по назначению
						if (cmd.Interface == CMD_INT_UART) {
							// пусто. так и надо.
						}
						if (cmd.Interface == CMD_INT_CAN) {
							printf(" CAN COPY %02X %02X %02X %02X",CAN2_Tx.Data[0],CAN2_Tx.Data[1],CAN2_Tx.Data[2],CAN2_Tx.Data[3]);
							printf(" %02X %02X %02X %02X \r\n",CAN2_Tx.Data[4],CAN2_Tx.Data[5],CAN2_Tx.Data[6],CAN2_Tx.Data[7]);			
							HAL_CAN_Transmit_IT(hcan2); // загрузили MAILBOX 0
						}
					}
				
				printf("CMD-STATUS DONE \r\n");						
			} 
			cmd.State = CMD_STATE_READY; // finish command interpretation, new command may be launched
		} // end of IF cmd.State == CMD_STATE_BUSY
		else {
		// cmd.State == CMD_STATE_READY, that means BUSY enter happened
		if (cmd.Interface == CMD_INT_UART) {
			err = CMD_ERROR_BUSY;
			printf(" COMMAND INTERPRETER IS BUSY, wait please \r\n");
			printf(" cmd.Code = %c %02X err = %02d \r\n",cmd.CmdCode,cmd.CmdCode,err);
		}
		if (cmd.Interface == CMD_INT_CAN) {
			err = CMD_ERROR_BUSY;
			CAN2_Tx.DLC = 2;
			CAN2_Tx.Data[0] = cmd.CmdCode;
			CAN2_Tx.Data[1] = err;
			HAL_CAN_Transmit_IT(hcan2);
			printf(" COMMAND INTERPRETER IS BUSY, wait please \r\n");
			printf(" cmd.Code = %s %02X err = %02d \r\n",&cmd.CmdCode,cmd.CmdCode,err);
			printf(" CAN COPY: %02X %02X \r\n",cmd.CmdCode,err);					
		}			
		}
			

	
	
				
	return err;
}


/**********************************/
/*        SWITCH COMMANDS		      */
/**********************************/

/**
 * \fn          int32_t check_switch(void)
 * \brief       Только при наличии переключателя на PB9 может появиться высокий уровень.
 *							Дергаю PB8 и смотрю на PB9.Если на PB9 уже был высокий уровень,он сбросится в 0,
 *							поэтому нужна еще одна проверка на 1,чтобы избежать этих ошибок.
 *							Я не знаю как еще обьяснить 
 */

int32_t check_switch(void)	{
	
	int32_t err = 1;
	
	status &= ~FLAG_SW_ACCESS;
	
	#ifdef SW_INSTALLED
	for (uint8_t gg=0; gg<3; gg++) {
		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_SET );
		HAL_Delay(1);
		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_RESET );
		HAL_Delay(10);
		uint8_t temp_sw_state1 = HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_9); 	  
	
	  HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_SET );
		HAL_Delay(1);
		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_RESET );
		HAL_Delay(10);
		uint8_t temp_sw_state2 = HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_9) ; 
		
		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_SET );
		HAL_Delay(1);
		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_RESET );
		HAL_Delay(10);
		uint8_t temp_sw_state3 = HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_9) ; 
			
			if( (temp_sw_state1 == temp_sw_state3) && (temp_sw_state1 != temp_sw_state2 ) ) {
				printf(" SWITCH->OK \r\n");
				status |= FLAG_SW_ACCESS;
				printf(" FLAG_SW_ACCESS-> DONE \r\n");
				gg = 3;
				err = 0;
			}
			else {
				printf(" SWITCH->ERR \r\n");
			}	
		}
	
	#else
		printf(" SWITCH-> NOT INSTALLED \r\n");
		status |= FLAG_SW_ACCESS;
		printf(" FLAG_SW_ACCESS-> DONE \r\n");
		err = 0;
	#endif

		return err;
};		

int32_t switch_cvt(void)	{	
	int32_t err = 1;
	
	#ifdef SW_INSTALLED
		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_SET );
		HAL_Delay(1);
		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_RESET );
		HAL_Delay(20);
		if (HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_9) == SW_STATE_CVT ){ 
			err = 0;	 
			printf("SWITCH->RUN_TO->CVT \r\n");
		}
		else	{ 
			HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_SET );
			HAL_Delay(1);
			HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_RESET );
			HAL_Delay(20);
			if (HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_9) == SW_STATE_CVT) {
				err = 0;	 
				printf("SWITCH->RUN_TO->CVT \r\n");
			}
			else {
				printf("SWITCH->CVT_ERROR \r\n");
			}
		}
		#else
			err = 0;
			printf("SWITCH->RUN_TO->CVT .. no SW installed \r\n");
		#endif
		return err;
};
	
int32_t switch_fbg(void)	{
	int32_t err = 1;
	
	#ifdef SW_INSTALLED
		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_SET );
		HAL_Delay(1);
		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_RESET );
		HAL_Delay(20);
		if (HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_9) == SW_STATE_FBG ){ 
			err = 0;
			printf("SWITCH->RUN_TO->FBG \r\n");
		}
		else	{ 
			HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_SET );
			HAL_Delay(1);
			HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, GPIO_PIN_RESET );
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_9) == SW_STATE_FBG) {
				err = 0;
				printf("SWITCH->RUN_TO->FBG \r\n");
			}
			else {
				printf("SWITCH->FBG_ERROR \r\n");
			}
		}
		#else
			err = 0;
			printf("SWITCH->RUN_TO->FBG .. no SW installed \r\n");
		#endif		
		return err;
};

/**********************************/
/*        LINE CALLING		        */
/**********************************/

int32_t scan(void) {
	
	uint16_t temp_line_center = TEMP_CENTER;
	
	
	if ( status | FLAG_CVT_IN_USE ) temp_line_center = TEMP_CVT_SCAN_CENTER; 
	if ( status | FLAG_FBG_IN_USE ) temp_line_center = TEMP_FBG_SCAN_CENTER; 
	
	
//		printf("SCAN IN PROCESS \r\n");

		if ( (ExtPD_mon > ExtPD_SETPOINT_DEPTH*ExtPD_outside) && (Tld_set > temp_line_center) )	{	
			if(tec_work==0) {
			Tld_set = Tld_set-10;
			TEC_Set(Tld_set);
			printf("ExtPD_mon = %d  0.9*ExtPD_outside = %7.2f  Tld_set = %d \r\n",ExtPD_mon, ExtPD_outside*ExtPD_SETPOINT_DEPTH, Tld_set);
			}
			else
			{printf("wAIT \r\n");}
		}
		if (Tld_set <= temp_line_center) {
			printf("Unsuccessful scan, no spec ref observed \r\n");
			status |= FLAG_SCAN_FAILED;
					scan_request=0; 
			return HAL_ERROR;
		}			
			if ( (ExtPD_mon <= ExtPD_SETPOINT_DEPTH*ExtPD_outside) && (Tld_set > temp_line_center) )	{	
		printf("SCAN SUCCESS \r\n");
		status |= FLAG_SCAN_SUCCESS;
		scan_request=0; 
		return HAL_OK;
			}
//				printf("NOTHING \r\n");
}

void SPI_DAC_Transmit(uint8_t f) {
	
	HAL_GPIO_WritePin( GPIOD, GPIO_PIN_2, GPIO_PIN_RESET );
	for(uint16_t k=0;k<100;k++){}
		uint8_t h=f>>8;
			HAL_SPI_Transmit(&hspi3, (uint8_t *)h, 1, 10);//одна 16ти битная передача???
	HAL_SPI_Transmit(&hspi3, &f, 1, 10);//одна 16ти битная передача???
	for(uint16_t k=0;k<100;k++){}	
  HAL_GPIO_WritePin( GPIOD, GPIO_PIN_2, GPIO_PIN_SET );
			for(uint16_t k=0;k<100;k++){}		
		  HAL_GPIO_WritePin( GPIOC, GPIO_PIN_7, GPIO_PIN_RESET );// обновление выходных данных dac специальной ножкой	
			for(uint16_t k=0;k<100;k++){}
		  HAL_GPIO_WritePin( GPIOC, GPIO_PIN_7, GPIO_PIN_SET );
				//вопрос как организовать передачу 16 бит,если это 1 переменная а размерность функции 8 бит
}
	
/**********************************/
/*  System Clock Configuration    */
/**********************************/

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 24;
  RCC_OscInitStruct.PLL.PLLN = 240;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);
}

void HAL_MspInit(void) {
	
	
}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef* hcan) {
	
	if ( hcan->Instance == CAN2)	{
		printf(" CAN2 Tx Finished \r\n");
		printf(" *************************** \r\n");	
	};
	
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan) { 
	
	uint8_t temp = 0;
	
	if ( hcan->Instance == CAN2)	{
		printf(" CAN2 Rx \r\n");
		if (cmd.State == CMD_STATE_READY) {
			cmd.CmdCode = CAN2_Rx.Data[0];
			cmd.Byte2 = CAN2_Rx.Data[1]; 				
			cmd.Byte3 = CAN2_Rx.Data[2]; 
			cmd.Interface = CMD_INT_CAN; 
			cmd.State = CMD_STATE_BUSY; 
			for (temp = 0; temp < COMM_WIDTH; temp++) {
				printf(" 0d%02d byte= 0x%02X \r\n",temp,CAN2_Rx.Data[temp]);
				CAN2_Rx.Data[temp] = 0;
			}
		}	
		cmd_interp(); // вызов командного интерпретатора		
	}
	
}
