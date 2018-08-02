// Header: SFCL_Driver_3_0 Firmware v1.0
// File Name: Main.h
// Abstract: Header for Main.c
// Author: D. Shelestov
// Date: 10.06.2015

#ifndef MAIN
#define MAIN

#include <stdint.h>
#include "stm32f207xx.h"
#include "stm32f2xx.h"
#include "Init.h"
//#include "IRQ.h"
#include "stdio.h"
#include "stdbool.h"

/*-----------------------------------------------------------------------------
  Personal Laser Settings
 *----------------------------------------------------------------------------*/

// Current DEVICE is ... choose ONLY ONE from the list
	// #define IRFS3_001
	// #define IRFS3_002
#define IRFS3_003

#ifdef IRFS3_003
	#define SW_INSTALLED  // раскомментить если установлен свич.
	#define SW_STATE_CVT 1  // проверено
	#define SW_STATE_FBG 0  // проверено
	//#define DAC_EX_INSTALLED // раскоментить если установ
	//#define PASSIVE_MODE
	
#endif



// Current LASER is ... choose ONLY ONE from the list
//#define SN_YE84079
#define SN_YE84080
//#define SN_YE84081
//#define SN_YE84082
//#define SN_YE84083

#ifdef SN_YE84079
	#define TLD_MIN_INIT 500
	#define TLD_MAX_INIT 3500
	
	#define TEMP_CVT_SCAN_INIT 2720 // J 170 000 lambda = center - 5 pm, scan start point (Fix ExtPD outside reference)
	#define TEMP_CVT_SCAN_LEFT 1280 // J 080 000
	#define TEMP_CVT_SCAN_CENTER 1280 // J 080 000
	#define TEMP_FBG_SCAN_INIT 2720 // J 170 000 lambda = center - 5 pm, scan start point (Fix ExtPD outside reference)
	#define TEMP_FBG_SCAN_LEFT 1280 // J 080 000
	#define TEMP_FBG_SCAN_CENTER 1280 // J 080 000
	#define TEMP_CENTER 320 // J 020 000
	#define TEMP_INIT 320 // J 020 000
	
	#define DC_INIT_C 1800 // E 110 000
	#define DC_INIT_P 1800 // E 110 000
	#define DC_SET_MAX_INIT 2500
	#define DC_SET_MIN_INIT 0
	
	#define ExtPD_SETPOINT_DEPTH 0.6 // 0.5 = -6 dB relative level  0.6 => -4.44 dB 0.7 => -3 dB
	#define ITEC_THR 4095 // Itec safety Threshold. if current Itec > Threshold, switch off TEC
	#define TEMP_EQU_PRECISION 2 // difference between Tld_real and Tld_setmon assumed as equal values
#endif

#ifdef SN_YE84080
	#define TLD_MIN_INIT 1400
	#define TLD_MAX_INIT 1800

	#define TEMP_CVT_SCAN_INIT 3400 // 4Ah(J) h(200d) 00h(000d) lambda = center - 5 pm, scan start point (Fix ExtPD outside reference)
																	// ~1542.380 nm
	#define TEMP_CVT_SCAN_LEFT 2560 // 4Ah(J) h(160d) 00h(000d)
	#define TEMP_CVT_SCAN_CENTER 2160 // 4Ah(J) h(135d) 00h(000d)
	#define TEMP_FBG_SCAN_INIT 2800 // J 253 000 lambda = center - 5 pm, scan start point (Fix ExtPD outside reference)
	#define TEMP_FBG_SCAN_LEFT 4045 // J 252 000
	#define TEMP_FBG_SCAN_CENTER 0 // J 105 000
	#define TEMP_CENTER 3200 // J 200 000
	#define TEMP_INIT 4050 // J 253 000
	
	#define DC_INIT_C 1800 // 45h(E) 6Eh(110d) 00h(000d)
	#define DC_INIT_P 1800 // 45h(E) 6Eh(110d) 00h(000d)
	#define DC_SET_MAX_INIT 2500
	#define DC_SET_MIN_INIT 0
	
	#define ExtPD_SETPOINT_DEPTH 0.9 // 0.5 = -6 dB relative level  0.6 => -4.44 dB 0.7 => -3 dB
	#define ITEC_THR 4095 // Itec safety Threshold. if current Itec > Threshold, switch off TEC
	#define CAN_FREQ_INIT 125
	#define TEMP_EQU_PRECISION 50 // difference between Tld_real and Tld_setmon assumed as equal values
#endif

/*-----------------------------------------------------------------------------
  FLASH Memory Addresses
 *----------------------------------------------------------------------------*/

	#define FLASH_DC_INIT_ADR	  0x0800C020
	#define FLASH_DC_MAX_ADR	  0x0800C040
	#define FLASH_DC_MIN_ADR	  0x0800C060
	#define FLASH_TEMP_INIT_ADR	0x0800C080
	#define FLASH_CAN_FREQ_ADR	0x0800C0A0
	#define FLASH_TEST_BYTE_ADR	0x0800C0C0
	
	#define FLASH_TEST_BYTE_VALUE 0x00AA // Значение тестового байта, никогда не меняется.
	

/*-----------------------------------------------------------------------------
  Code MODES
 *----------------------------------------------------------------------------*/
//#define _UART_CONSOLE_SIMPLE // enable serive info through USART, use if no retargeting organized
#define _UART_CONSOLE_FULL // enable service info through USART, use if Retarget.c launched in project
#define _SPI_CONSOLE // enable serive info through SPI 
#define _SPI_CONSOLE

/*-----------------------------------------------------------------------------
  Exported types
 *----------------------------------------------------------------------------*/

/** 
  * @brief Command Interpreter Possible States
  */ 
typedef enum
{
  CMD_STATE_READY             = 0x00,    /*!< Command Interpreter ready to get command           */
	CMD_STATE_BUSY             = 0x01,     /*!< Previous command not finished, new one will be neglected    */
  
}CMD_StateTypeDef;

/** 
  * @brief Interfaces used in command transaction
  */ 
typedef enum
{
  CMD_INT_UART             = 0x00,    /*!< Command Received via UART bus   */
	CMD_INT_CAN             = 0x01,     /*!< Command Received via CAN bus    */
  
}CMD_IntTypeDef;

/** 
  * @brief Command Interpreter Error Codes
  */ 
typedef enum
{
  CMD_ERROR_NONE             = 0x00,    /*!< Command Interpreter finished successfully  */
	CMD_ERROR_NOCMD            = 0x01,    /*!< Received Command Code doesn't exist */
	CMD_ERROR_BLOCKED	         = 0x02,    /*!< Command blocked in USER mode */
	CMD_ERROR_OOR              = 0x03,    /*!< Command value is Out Of Rage	*/
	CMD_ERROR_BUSY             = 0x04,    /*!< Interpreter is busy, command ignored	*/
	CMD_ERROR_SYSERR           = 0x05,    /*!< Command cannot be performed, because some SYSTEM ERROR happened */
	
}CMD_ErrorTypeDef;

/** 
  * @brief External Command Structure definition  
  */ 
typedef struct
{
  uint8_t CmdCode;                  /*!< Command Code - unique command name used in any place of program.
                                         Possible names are (see PROTOCOL v1.1+)  'A' to 'R'
																				 Activity which is need to be done in any case is written in cmd_interp() function	*/
	
  uint8_t Byte2;                		/*!< Specifies additional data for command according to PROTOCOL 
                                           This parameter can be 0 when unused  */
	
  uint8_t Byte3;                  	/*!< Specifies additional data for command according to PROTOCOL 
                                           This parameter can be 0 when unused  */
	
	CMD_IntTypeDef Interface;					/*!< Specifies current state of command unterpreter - READY or BUSY   */
	
	CMD_StateTypeDef State;						/*!< Specifies current state of command unterpreter - READY or BUSY   */
	
}CMD_TypeDef;


/*-----------------------------------------------------------------------------
  Possible Laser States 
 *----------------------------------------------------------------------------*/
// Constants Naming in KEIL
// Binary:	None	Y or y	11111111Y
// Decimal:	None	T or none	1234T or 1234
// Hexadecimal:	0x or 0X	H or h	1234H or 0x1234
// Octal:	0 (zero)	Q, q, O, or o	0777 or 777q or 777Q or 777o

#define STATE_1					 0x0000  // 0000_0000_0000_0000y
#define STATE_1_MASK		 0x57E7  // 0101_0111_1110_0111y               
#define STATE_2  				 0x0801  // 0000_0000_0000_0001y
#define STATE_2_MASK		 0xFFEF  // 1111_1111_1110_1111y

#define STATE_3 			   0x0020  // 0000_0000_0010_0000y               
#define STATE_3_MASK		 0x57E6  // 0101_0111_1110_0110y
#define STATE_4          0x0431  // 0000_0100_0011_0001y
#define STATE_4_MASK		 0x57F7  // 0101_0111_1111_0111y

#define STATE_5				   0x0025  // 0000_0000_0010_0101y
#define STATE_5_MASK		 0x57EF  // 0101_0111_1110_1111y

#define STATE_6  	       0x4339  // 0100_0011_0011_1001y
#define STATE_6_MASK		 0x57FF  // 0101_0111_1111_1111y

#define STATE_7	         0x1339  // 0001_0011_0011_1001y
#define STATE_7_MASK		 0x57FF  // 0101_0111_1111_1111y
#define STATE_8			  	 0x4139  // 0100_0001_0011_1001y
#define STATE_8_MASK		 0x57FF  // 0101_0111_1111_1111y

#define STATE_9		       0x1139  // 0001_0001_0011_1001y
#define STATE_9_MASK		 0x57FF  // 0101_0111_1111_1111y
#define STATE_10		     0x4179  // 0100_0001_0111_1001y
#define STATE_10_MASK		 0x57FF  // 0101_0111_1111_1111y

#define STATE_11	 	     0x1179  // 0001_0001_0111_1001y
#define STATE_11_MASK		 0x57FF  // 0101_0111_1111_1111y
#define STATE_12  	     0x407B  // 0100_0000_0111_1011y
#define STATE_12_MASK		 0x57FF  // 0101_0111_1111_1111y

#define STATE_13         0x107B  // 0001_0000_0111_1011y
#define STATE_13_MASK		 0x57FF  // 0101_0111_1111_1111y


/*-----------------------------------------------------------------------------
 Переменные для  работы команд с состояниями
 *----------------------------------------------------------------------------*/

#define STATE_1_LASER_OFF                    1

#define STATE_2_INIT  				               2
#define STATE_3_TEC_START			               3
#define STATE_4_PASSIVE                      4
#define STATE_5_PWR_CHANGE				           5

#define STATE_6_CVT_SCAN_INIT  	             6
#define STATE_7_FBG_SCAN_INIT	               7

#define STATE_8_CVT_SCAN			  	           8
#define STATE_9_FBG_SCAN			               9

#define STATE_10_CVT_STAB_WAIT		          10
#define STATE_11_FBG_STAB_WAIT		       	  11

#define STATE_12_CVT_STAB_WORK  	          12
#define STATE_13_FBG_STAB_WORK              13


/*-----------------------------------------------------------------------------
	Laser Control/Status Register   'status'
 *----------------------------------------------------------------------------*/
#define FLAG_CVT (uint16_t)					0x8000	// 0 - cuvette mode not activated, 1 - cuvette activated
#define FLAG_CVT_IN_USE (uint16_t)	0x4000	// 0 - cuvette not used, 1 - cuvette in use 
#define FLAG_FBG (uint16_t)					0x2000	// 0 - Bragg Grating not activated, 1 - Brag Grating activated
#define FLAG_FBG_IN_USE (uint16_t)	0x1000	// 0 - Bragg grating not used, 1 - Bragg Grating in use 
#define FLAG_AUTO_STB (uint16_t)		0x0800	// 0 - manual change spectral reference (bits 16:15), 1 - automatic spec ref
#define FLAG_PSV_IN_USE (uint16_t)	0x0400	// 0 - ACTIVE stabilisation in use, 1 - PASSIVE stabilisation in use // READ ONLY
#define FLAG_SCAN_INIT (uint16_t)		0x0200	// 0 - temperature is out scan init point, 1 - temperature init point set
#define FLAG_SCAN (uint16_t)				0x0100	// 0 - no scan, 1 - scanning in process
#define FLAG_SCAN_FAILED (uint16_t)	0x0080	// 0 - scan works properly, 1 - full range scanned, no spec ref detected
#define FLAG_SCAN_SUCCESS (uint16_t)0x0040	// 0 - no line detected, 1 - scan leaded to active spec ref capture
#define FLAG_TEC_ON (uint16_t)			0x0020	// 0 - Peltier Cooler(TEC) is OFF, 1 - TEC is on
#define FLAG_TEMP_EQU (uint16_t)		0x0010	// 0 - |Tld_real - Tld_setmon| > 50; 1 - |Tld_real - Tld_setmon| < 50

/* Tld_real and Tld_setmon - voltages on two symmetric pointes of temp measurement bridge.
	 In normal work their difference have to be no more then stability error (for example, 50)
	 Huge difference occurs when step change apears on Tld_setmon, TEC needs time (up to 5 min) to make them equal	*/
#define FLAG_SW_ACCESS (uint16_t)		0x0008	// 0 - no access to SW, 1 - SW controlled
#define FLAG_PWR_UPDOWN (uint16_t)	0x0004	// 0 - LD power level is constant, 1 - LD power is rising or falling (up to 2 min)

#define FLAG_CAPTURE (uint16_t)			0x0002	// 0 - reference not captured, 1 - reference captured
#define FLAG_LASER_ON (uint16_t)		0x0001	// 0 - Laser OFF, Stabilization suspended 1 - Laser ON, NORMAL MODE

/*-----------------------------------------------------------------------------
	System Error Register
 *----------------------------------------------------------------------------*/

#define SYSERR_CVT_CAPT 	(uint8_t)				0x80	// 0 - cuvette OK, 1 - last attempt to capture CVT failed
#define SYSERR_FBG_CAPT 	(uint8_t)				0x40	// 0 - Bragg grating OK, 1 - last attempt to capture FBG failed
#define SYSERR_SW_ACCESS 	(uint8_t)				0x20	// 0 - SW works properly, 1 - SW is needed my Laser, but it's not detected
#define SYSERR_TEC_OVRC 	(uint8_t)				0x10	// 0 - TEC current is OK, 1 - Overcurrent in TEC opamp detected
#define SYSERR_STATE 			(uint8_t)				0x08	// 0 - states change correctly, 1 - During enter the state error happened
// if happens - lasers stop changing states until SYSERR register reset by user
#define SYSERR_LAS_OVRT		(uint8_t)				0x04	// 0 - states change correctly, 1 - For some reason laser was over/under-heated
#define SYSERR_FLASH			(uint8_t)				0x02	// 0 - load from FLASH OK, 1 - last load from FLASH failed, default settings in use
// if happens - laser switches off and stop 

/*-----------------------------------------------------------------------------
   Optical Switch States
 *----------------------------------------------------------------------------*/
#define OS_STATE_CVT (uint8_t)		0x00	// switch in CUVETTE position
#define OS_STATE_FBG (uint8_t)		0x01	// switch in BRAGG GRATING position

/*-----------------------------------------------------------------------------
   Functions
 *----------------------------------------------------------------------------*/
int32_t cmd_interp(void);//(uint8_t *cmd_ptr);
int32_t new_state(uint8_t stat_new); // switch system to new state
int32_t check_switch(void); // check whether switch working or not
int32_t switch_fbg(void); 
int32_t switch_cvt(void);
int32_t scan(void);
void SPI_DAC_Transmit(uint8_t f);
void SystemClock_Config(void);
//void ADC_Update(void);

#endif
