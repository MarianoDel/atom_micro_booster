//---------------------------------------------
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    STM32F030
// ##
// #### HARD.C ################################
//---------------------------------------------

/* Includes ------------------------------------------------------------------*/
#include "hard.h"
#include "tim.h"
#include "stm32f0xx.h"
#include "adc.h"
#include "dsp.h"

#include <stdio.h>

/* Externals variables ---------------------------------------------------------*/
extern volatile unsigned short timer_led;
extern volatile unsigned short adc_ch[];




/* Global variables ------------------------------------------------------------*/
//para el led
led_state_t led_state = START_BLINKING;
unsigned char blink = 0;
unsigned char how_many_blinks = 0;



/* Module Functions ------------------------------------------------------------*/


#ifdef WITH_HYST
unsigned short GetHysteresis (unsigned char hours_past)
{
	if (hours_past > 8)
		return HYST_MIN;
	else if (hours_past > 6)
		return HYST_6;
	else if (hours_past > 4)
		return HYST_4;
	else if (hours_past > 2)
		return HYST_2;
	else
		return HYST_MAX;
}
#endif

#ifdef WITH_1_TO_10_VOLTS
unsigned char GetNew1to10 (unsigned short light)	//prendo en 3722 a 4095 tengo 373 puntos
{
	unsigned short new_light = 0;

	if (light > VOLTAGE_PHOTO_ON)
	{
		new_light = light - VOLTAGE_PHOTO_ON;
	}
	new_light += PWM_MIN;

	if (new_light > 255)
		new_light = 255;

	// if (light < VOLTAGE_PHOTO_ON)
	// 	new_light = PWM_MIN;
	// else
	// {
	// 	new_light = light - VOLTAGE_PHOTO_ON;
	// 	new_light += PWM_MIN;
	// }

	return (unsigned char) new_light;
}
#endif


//cambia configuracion de bips del LED
void ChangeLed (unsigned char how_many)
{
    how_many_blinks = how_many;
    led_state = START_BLINKING;
}

//mueve el LED segun el estado del Pote
void UpdateLed (void)
{
    switch (led_state)
    {
        case START_BLINKING:
            blink = how_many_blinks;
            
            if (blink)
            {
                LED_ON;
                timer_led = 200;
                led_state++;
                blink--;
            }
            break;

        case WAIT_TO_OFF:
            if (!timer_led)
            {
                LED_OFF;
                timer_led = 200;
                led_state++;
            }
            break;

        case WAIT_TO_ON:
            if (!timer_led)
            {
                if (blink)
                {
                    blink--;
                    timer_led = 200;
                    led_state = WAIT_TO_OFF;
                    LED_ON;
                }
                else
                {
                    led_state = WAIT_NEW_CYCLE;
                    timer_led = 2000;
                }
            }
            break;

        case WAIT_NEW_CYCLE:
            if (!timer_led)
                led_state = START_BLINKING;

            break;

        default:
            led_state = START_BLINKING;
            break;
    }
}

//proteccion para no superar el valor Vin . Ton que puede saturar al trafo
//con 6T primario
unsigned short UpdateDMAX (unsigned short a)
{
    if (a > VIN_35V)
        a = 260;
    else if (a > VIN_30V)
        a = 297;
    else if (a > VIN_25V)
        a = 347;
    else if (a > VIN_20V)
        a = 417;
    else
        a = 450;
    
    return a;
}


//---- end of file ----//
