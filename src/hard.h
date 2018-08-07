//---------------------------------------------
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    STM32F103
// ##
// #### DSP.H ################################
//---------------------------------------------
#ifndef HARD_H_
#define HARD_H_


//----------- Defines For Configuration -------------
//----------- Hardware Board Version -------------
// #define VER_1_0
#define VER_1_1		//cambia pinout respecto VER_1_0

//---- Configuration for Hardware Versions -------
#ifdef VER_1_1
#define HARDWARE_VERSION_1_1
#define SOFTWARE_VERSION_1_1
#endif
#ifdef VER_1_0
#define HARDWARE_VERSION_1_0
#define SOFTWARE_VERSION_1_0
#endif


//---- Features Configuration ----------------
// #define DEBUG_ON
// #define PRODUCTION_PRGRM
// #define TEST_INT_PRGRM
// #define TEST_ADC_AND_DMA
// #define TEST_FIXED_D
#define TEST_FIXED_VOUT


//------ Configuration for Firmware-Channels -----


//---- End of Features Configuration ----------



//--- Hardware Welcome Code ------------------//
#ifdef HARDWARE_VERSION_1_1
#define HARD "Hardware V: 1.1\n"
#endif
#ifdef HARDWARE_VERSION_1_0
#define HARD "Hardware V: 1.0\n"
#endif

//--- Software Welcome Code ------------------//
#ifdef SOFTWARE_VERSION_1_2
#define SOFT "Software V: 1.2\n"
#endif
#ifdef SOFTWARE_VERSION_1_1
#define SOFT "Software V: 1.1\n"
#endif
#ifdef SOFTWARE_VERSION_1_0
#define SOFT "Software V: 1.0\n"
#endif

//--- Type of Program Announcement ----------------
#ifdef TEST_INT_PRGRM
#define FEATURES "Programa de Testeo INT\n"
#endif
#ifdef TEST_ADC_AND_DMA
#define FEATURES "Programa de Testeo ADC -> DMA\n"
#endif
#ifdef TEST_FIXED_D
#define FEATURES "Programa de ciclo d fijo\n"
#endif
#ifdef TEST_FIXED_VOUT
#define FEATURES "Programa Vout fijo\n"
#endif



//-------- Others Configurations depending on the formers ------------
//-------- Hysteresis Conf ------------------------

//-------- PWM Conf ------------------------

//-------- End Of Defines For Configuration ------

#define VIN_35V    986
#define VIN_30V    845
#define VIN_25V    704
#define VIN_20V    561    //1.81V

// #define VOUT_200V    415
#define VOUT_110V    151    //ajustado 05-08-18
#define VOUT_200V    386    //ajustado 24-07-18
#define VOUT_300V    660    //ajustado 24-07-18
#define VOUT_350V    802    //ajustado 24-07-18

//Caracteristicas de la bobina de salida
#define LOUT_UHY    130    //DINL2
#define ILOUT       3      //DINL2 corriente un poco menor a la que satura el inductor
#define TICK_PWM_NS 21
#define N_TRAFO     18300
#define IMAX_INPUT  25
#define MAX_VOUT    830    //830 -> 362V tension maxima que sale del trafo en puntos ADC

#if ((ILOUT * N_TRAFO) > (IMAX_INPUT * 1000))
#define I_FOR_CALC_MILLIS (IMAX_INPUT * 1000 * 1000 / N_TRAFO)
#define I_FOR_CALC (IMAX_INPUT * 1000 / N_TRAFO)
#else
#define I_FOR_CALC_MILLIS (ILOUT * 1000 * 1000)
#define I_FOR_CALC (IMAX_INPUT * 1000)
#endif



#define DMAX_HARDWARE    450

#ifdef TEST_FIXED_D
#define D_FOR_FIXED    20
#endif

//------- PIN CONFIG ----------------------
#ifdef VER_1_1
//GPIOA pin0	Vin_Sense
//GPIOA pin1	Vout_Sense
//GPIOA pin2	I_Sense_MOS_A
//GPIOA pin3	I_Sense_MOS_B

//GPIOA pin4	
#define PROT_MOS_A	((GPIOA->IDR & 0x0010) != 0)
//GPIOA pin5	
#define PROT_MOS_B	((GPIOA->IDR & 0x0020) != 0)

//GPIOA pin6	para TIM3_CH1
//GPIOA pin7	NC

//GPIOB pin0    NC
//GPIOB pin1	NC

//GPIOA pin8	para TIM1_CH1

//GPIOA pin9
//GPIOA pin10	usart1 tx rx

//GPIOA pin11	NC
//GPIOA pin12	NC
//GPIOA pin13	NC
//GPIOA pin14	NC

//GPIOA pin15
#define LED ((GPIOA->ODR & 0x8000) != 0)
#define LED_ON	GPIOA->BSRR = 0x00008000
#define LED_OFF GPIOA->BSRR = 0x80000000

//GPIOB pin3	NC
//GPIOB pin4	NC
//GPIOB pin5	NC

//GPIOB pin6
#define STOP_JUMPER ((GPIOB->IDR & 0x0040) == 0)

//GPIOB pin7	NC
#endif	//VER_1_1

#ifdef VER_1_0
//GPIOA pin0	Vin_Sense
//GPIOA pin1	Vout_Sense
//GPIOA pin2	I_Sense
//GPIOA pin2	para pruebas
// #define ISENSE_ON	GPIOA->BSRR = 0x00000004
// #define ISENSE_OFF GPIOA->BSRR = 0x00040000


//GPIOA pin3	NC
//GPIOA pin4	NC
//GPIOA pin5	NC

//GPIOA pin6	para TIM3_CH1
//GPIOA pin7	NC

//GPIOB pin0
#define OVERCURRENT	((GPIOB->IDR & 0x0001) == 0)

//GPIOB pin1	TIM14_CH1 o TIM3_CH4

//GPIOA pin8	para TIM1_CH1
// #define CTRL_M_B_ON	GPIOA->BSRR = 0x00000100
// #define CTRL_M_B_OFF GPIOA->BSRR = 0x01000000


//GPIOA pin9
//GPIOA pin10	usart1 tx rx

//GPIOA pin11	NC
//GPIOA pin12	NC
//GPIOA pin13	NC
//GPIOA pin14	NC

//GPIOA pin15
#define LED ((GPIOA->ODR & 0x8000) != 0)
#define LED_ON	GPIOA->BSRR = 0x00008000
#define LED_OFF GPIOA->BSRR = 0x80000000

//GPIOB pin3	NC
//GPIOB pin4	NC
//GPIOB pin5	NC

//GPIOB pin6
#define STOP_JUMPER ((GPIOB->IDR & 0x0040) == 0)

//GPIOB pin7	NC
 #endif	//VER_1_0
//------- END OF PIN CONFIG -------------------



//ESTADOS DEL PROGRAMA PRINCIPAL
typedef enum
{
    MAIN_INIT = 0,
    MAIN_SOFT_START,
    MAIN_GENERATING,
    MAIN_OVERCURRENT,
    MAIN_JUMPER_PROTECTED,
    MAIN_GO_TO_FAILURE,
    MAINS_FAILURE

} main_state_t;

//ESTADOS DEL LED
typedef enum
{    
    START_BLINKING = 0,
    WAIT_TO_OFF,
    WAIT_TO_ON,
    WAIT_NEW_CYCLE
} led_state_t;


//Estados Externos de LED BLINKING
#define LED_NO_BLINKING               0
#define LED_STANDBY                   1
#define LED_GENERATING                2
#define LED_LOW_VOLTAGE               3
#define LED_PROTECTED                 4
#define LED_VIN_ERROR                 5
#define LED_OVERCURRENT_ERROR         6


#define SIZEOF_DATA1	512
#define SIZEOF_DATA		256
#define SIZEOF_DATA512	SIZEOF_DATA1
#define SIZEOF_DATA256	SIZEOF_DATA
#define SIZEOF_BUFFTCP	SIZEOF_DATA





/* Module Functions ------------------------------------------------------------*/
unsigned short GetHysteresis (unsigned char);
unsigned char GetNew1to10 (unsigned short);
void UpdateVGrid (void);
void UpdateIGrid (void);
unsigned short GetVGrid (void);
unsigned short GetIGrid (void);
unsigned short PowerCalc (unsigned short, unsigned short);
unsigned short PowerCalcMean8 (unsigned short * p);
void ShowPower (char *, unsigned short, unsigned int, unsigned int);
void ChangeLed (unsigned char);
void UpdateLed (void);
unsigned short UpdateDMAX (unsigned short);
unsigned short UpdateDMAXSF (unsigned short);
unsigned short UpdateDmaxLout (unsigned short);
unsigned short VoutTicksToVoltage (unsigned short);
unsigned short VinTicksToVoltage (unsigned short);
    
#endif /* HARD_H_ */
