//-----------------------------------------------------
// #### MICROINVERTER PROJECT - Custom Board ####
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    STM32F030
// ##
// #### MAIN.C ########################################
//-----------------------------------------------------

// Includes --------------------------------------------------------------------
#include "stm32f0xx.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gpio.h"
#include "tim.h"
#include "uart.h"
#include "hard.h"
#include "test_functions.h"
#include "boost.h"

// #include "core_cm0.h"
#include "adc.h"
#include "dma.h"
#include "flash_program.h"
#include "dsp.h"

#include "it.h"


// Externals -------------------------------------------------------------------
// -- Externals from serial
volatile unsigned char tx1buff[SIZEOF_DATA];
volatile unsigned char rx1buff[SIZEOF_DATA];
volatile unsigned char usart1_have_data = 0;

// -- Externals from ADC 
volatile unsigned short adc_ch[ADC_CHANNEL_QUANTITY];
volatile unsigned char seq_ready = 0;

// -- Externals from timers
volatile unsigned short timer_led = 0;

// -- Externals from filters
volatile unsigned short take_temp_sample = 0;
volatile unsigned char hard_overcurrent = 0;

// ------- Definiciones para los filtros -------
#define SIZEOF_FILTER    8
unsigned short vin_vector [SIZEOF_FILTER];
#ifdef USE_FREQ_75KHZ
#define UNDERSAMPLING_TICKS    45
#define UNDERSAMPLING_TICKS_SOFT_START    90
#endif
#ifdef USE_FREQ_48KHZ
#define UNDERSAMPLING_TICKS    10
#define UNDERSAMPLING_TICKS_SOFT_START    20
#endif


// Globals ---------------------------------------------------------------------
volatile unsigned char current_excess = 0;
volatile short d = 0;
short ez1 = 0;
short ez2 = 0;
// unsigned short dmax = 0;
unsigned short last_d = 0;
#define DELTA_D    2

// -- Globals for timers
volatile unsigned short wait_ms_var = 0;
volatile unsigned short timer_standby;
//volatile unsigned char display_timer;
volatile unsigned short timer_meas;
volatile unsigned char timer_filters = 0;

volatile unsigned short dmax_permited = 0;
// volatile unsigned short secs = 0;
// volatile unsigned char hours = 0;
// volatile unsigned char minutes = 0;






// Module Private Functions ----------------------------------------------------
void TimingDelay_Decrement (void);
void SysTickError (void);

#ifdef VER_1_2
// ------- para el LM393 -------
void EXTI4_15_IRQHandler(void);
#endif

#ifdef VER_2_0
// ------- para el LM311 -------
extern void EXTI4_15_IRQHandler(void);
#endif


// Module Functions ------------------------------------------------------------
//-------------------------------------------//
// @brief  Main program.
// @param  None
// @retval None
//------------------------------------------//
int main(void)
{
    //GPIO Configuration.
    GPIO_Config();

    //Start the SysTick Timer
    if (SysTick_Config(48000))
        SysTickError();
    
    //--- Hardware Tests Functions ---
    // TF_Led ();
    // TF_Led_Jumper ();
    // TF_Led_Blinking();
    // TF_Usart1_Tx ();
    // TF_Tim_Channels ();
    // TF_Prot_Mosfet ();
    // TF_Prot_Mosfet_Int ();    
    TF_Usart1_Adc_Dma ();    
    //--- End of Hardware Tests Functions ---    
    
    //--- Welcome code ---//
    Usart1Config();
    EXTIOff();

#ifdef FEATURES
    WelcomeCodeFeatures ();
#endif
//---------- Version 1_2  --------//

    // Enable the Hard needed
    // TIM Config
    TIM_1_Init ();    // mosfet Ctrol_M_B & synchro TIM3 & synchro ADC
    TIM_3_Init ();    // mosfet Ctrol_M_A

    EnablePreload_MosfetA;
    EnablePreload_MosfetB;

    UpdateTIMSync (DUTY_NONE);

    // ADC & DMA Config
    AdcConfig();
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;
    ADC1->CR |= ADC_CR_ADSTART;

    //Init the boost filters
    BoostFiltersInit ();
    
    while (1)
    {
        BoostLoop();
        UpdateLed();
    }
    
//---------- End of Version 1_2  --------//

    
//---------- Version 2_0  --------//    
#if (defined VER_2_0)
    unsigned char i;
    unsigned short ii;

    unsigned char undersampling = 0;
    main_state_t main_state = MAIN_INIT;
    unsigned short vin_filtered = 0;

    unsigned short dmax_lout = 0;
    unsigned short dmax_vin = 0;    

#ifdef TEST_FIXED_D
    unsigned char stopped = 0;
#endif

#ifdef ONLY_COMMS
    unsigned short dmax_vin = 0;
#endif
    char s_lcd [120];		    
    
    TIM_1_Init ();	   //lo utilizo para mosfet Ctrol_M_B y para FB si esta definido en hard.h
    TIM_3_Init ();	   //lo utilizo para mosfet Ctrol_M_A y para synchro ADC

    EnablePreload_MosfetA;
    EnablePreload_MosfetB;

#ifdef WITH_TIM14_FB
    TIM_14_Init ();        //lo uso para FB
#endif
    

    AdcConfig();

    //-- DMA configuration.
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;
                

#ifdef USE_PUSH_PULL_MODE
    //uso los dos mosfets, TIM1 mosfet B, TIM3 mosfet A
    timer_standby = 2000;
#ifdef USE_LED_IN_PROT
    LED_OFF;
#endif
    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (!timer_standby)
            {
                d = 0;
                UpdateTIMSync(0);
                EXTIOn();
                //si no le pongo esto puede que no arranque
                UpdateFB(DUTY_FB_25A);
                main_state = MAIN_SOFT_START;
                LED_OFF;
            }

            if (sequence_ready)
                sequence_ready_reset;
            break;

        case MAIN_SOFT_START:
            if (sequence_ready)
                sequence_ready_reset;

#ifdef IN_PUSH_PULL_GROW_TO_FIXED_D
            if (!timer_standby)
            {
                timer_standby = 2;
                if (d < DUTY_FOR_DMAX)
                {
                    d++;
                    UpdateTIMSync(d);
                }
                else
                {
                    main_state = MAIN_VOLTAGE_MODE;                    
                }
            }
#endif
#ifdef IN_PUSH_PULL_SET_FIXED_D
            UpdateTIMSync(DUTY_FOR_DMAX);
            main_state = MAIN_VOLTAGE_MODE;                    
#endif
#ifdef IN_PUSH_PULL_VM
            //aumento el ciclo de trabajo si estoy debajo del maximo
            //o si me falta para llegar al seteo de Vout
            if (!timer_standby)
            {
                timer_standby = 2;
                if ((d < DUTY_FOR_DMAX) && (Vout_Sense < VOUT_SETPOINT))
                {
                    d++;
                    UpdateTIMSync(d);
                }
                else
                {
                    main_state = MAIN_VOLTAGE_MODE;                    
                }
            }
#endif
            break;
            
        case MAIN_VOLTAGE_MODE:
#ifdef IN_PUSH_PULL_VM
            if (sequence_ready)
            {
                sequence_ready_reset;
                
                if (undersampling < (UNDERSAMPLING_TICKS - 1))
                    undersampling++;
                else
                {
                    undersampling = 0;
                    d = PID_roof (VOUT_SETPOINT, Vout_Sense, d, &ez1, &ez2);
                    
                    if (d < 0)
                    {
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                    }
                    UpdateTIMSync (d);
                }
            }            
#else            
            if (sequence_ready)
                sequence_ready_reset;
#endif
            break;

        case MAIN_OVERVOLTAGE:
            if ((!timer_standby) && (sequence_ready))
            {
                sequence_ready_reset;
                if (Vout_Sense < VOUT_OVERVOLTAGE_THRESHOLD_TO_RECONNECT)
                {
                    main_state = MAIN_INIT;
                    Usart1Send((char *) "Reconnect...\n");
                }
            }
            break;

        case MAIN_UNDERVOLTAGE:
            if ((!timer_standby) && (sequence_ready))
            {
                sequence_ready_reset;
                if (vin_filtered > VIN_UNDERVOLTAGE_THRESHOLD_TO_RECONNECT)
                {
                    main_state = MAIN_INIT;
                    Usart1Send((char *) "Reconnect...\n");
                }
            }
            break;
            
        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    main_state = MAIN_JUMPER_PROTECT_OFF;
                    timer_standby = 400;
                }
            }                
            break;

        case MAIN_JUMPER_PROTECT_OFF:
            if (!timer_standby)
            {
                //vuelvo a INIT
                main_state = MAIN_INIT;
                Usart1Send((char *) "Protect OFF\n");                    
            }                
            break;

        case MAIN_OVERCURRENT:
            if ((!timer_standby) && (STOP_JUMPER))
            {
                Usart1Send((char *) "leaving overcurrent to jumper prot!\n");
                main_state = MAIN_JUMPER_PROTECTED;
                timer_standby = 1000;
            }
            break;

        default:
            main_state = MAIN_INIT;
            break;
        }	//fin switch main_state
        
        //Cosas que no tienen tanto que ver con las muestras o el estado del programa
        if ((STOP_JUMPER) &&
            (main_state != MAIN_JUMPER_PROTECTED) &&
            (main_state != MAIN_JUMPER_PROTECT_OFF) &&            
            (main_state != MAIN_OVERCURRENT))
        {
            UpdateTIMSync(DUTY_NONE);
            UpdateFB(DUTY_NONE);
            EXTIOff();
            main_state = MAIN_OVERVOLTAGE;
            Usart1Send((char *) "Protect ON\n");
            timer_standby = 1000;
            main_state = MAIN_JUMPER_PROTECTED;
        }

        //si esta corriendo reviso tension de salida y entrada
        if ((main_state == MAIN_SOFT_START) ||
            (main_state == MAIN_VOLTAGE_MODE))
        {
            //proteccion de sobretension
            if (Vout_Sense > VOUT_OVERVOLTAGE_THRESHOLD_TO_DISCONNECT)
            {
                UpdateTIMSync(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                EXTIOff();
#ifdef USE_LED_IN_PROT
                LED_ON;
#endif
                main_state = MAIN_OVERVOLTAGE;
                sprintf (s_lcd, "Overvoltage! Vout: %d d: %d\n", Vout_Sense, d);
                Usart1Send(s_lcd);
                
                timer_standby = 1000;
            }

            //proteccion de falta de tension
            if (vin_filtered < VIN_UNDERVOLTAGE_THRESHOLD_TO_DISCONNECT)            
            // if (Vin_Sense < VIN_UNDERVOLTAGE_THRESHOLD_TO_DISCONNECT)
            {
                UpdateTIMSync(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                EXTIOff();
#ifdef USE_LED_IN_PROT
                LED_ON;
#endif
                main_state = MAIN_UNDERVOLTAGE;
                // sprintf (s_lcd, "Undervoltage! VM: %d\n", Vin_Sense);
                sprintf (s_lcd, "Undervoltage! Vin: %d d: %d\n", vin_filtered, d);
                Usart1Send(s_lcd);

                timer_standby = 4000;                
            }
        }

        if (current_excess)
        {
            current_excess = 0;
            Usart1Send((char *) "overcurrent\n");
            main_state = MAIN_OVERCURRENT;
            timer_standby = 2000;
        }

        if (!timer_meas)
        {
            timer_meas = 2000;
            sprintf (s_lcd, "Vin: %d, Vout: %d, d: %d, dmax_vin: %d, dmax_lout: %d\n",
                     vin_filtered,
                     Vout_Sense,
                     d,
                     dmax_vin,
                     dmax_lout);
            
            Usart1Send(s_lcd);
        }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
            dmax_vin = UpdateDMAX(vin_filtered);
        }
    }
#endif    //USE_PUSH_PULL_MODE

#ifdef USE_FORWARD_MODE
    //uso solo mosfet de TIM3, mosfet A
    timer_standby = 2000;
#ifdef USE_LED_IN_PROT
    LED_OFF;
#endif
    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (!timer_standby)
            {
                d = 0;
                UpdateTIM_MosfetA(0);                
                EXTIOn();
                //si no le pongo esto puede que no arranque
                UpdateFB(DUTY_FB_25A);
                main_state = MAIN_SOFT_START;
                LED_OFF;
            }

            if (sequence_ready)
                sequence_ready_reset;
            break;

        case MAIN_SOFT_START:
            if (sequence_ready)
            {
                sequence_ready_reset;

                if (undersampling < (UNDERSAMPLING_TICKS_SOFT_START - 1))
                    undersampling++;
                else
                {
                    undersampling = 0;
                    if (VOUT_SETPOINT < Vout_Sense)
                        d++;
                    else
                    {
                        main_state = MAIN_VOLTAGE_MODE;
                        break;
                    }

                    dmax_lout = Hard_GetDmaxLout (Vin_Sense, Vout_Sense);
                    
                    //maximos del pwm por corriente en bobina de salida
                    //o saturacion de trafo por tension de entrada
                    if (dmax_vin > dmax_lout)
                    {
                        //dmax por corriente out
                        if (d > dmax_lout)
                            d = dmax_lout;
                    }
                    else
                    {
                        //dmax por vin
                        if (d > dmax_vin)
                            d = dmax_vin;
                    }
                    
                    UpdateTIM_MosfetA(d);
                }
            }            
            break;
            
        case MAIN_VOLTAGE_MODE:
            if (sequence_ready)
            {
                sequence_ready_reset;

                if (undersampling < (UNDERSAMPLING_TICKS - 1))
                    undersampling++;
                else
                {
                    undersampling = 0;
                    d = PID_roof (VOUT_SETPOINT, Vout_Sense, d, &ez1, &ez2);                    

                    dmax_lout = Hard_GetDmaxLout (Vin_Sense, Vout_Sense);
                    
                    //maximos del pwm por corriente en bobina de salida
                    //o saturacion de trafo por tension de entrada
                    if (dmax_vin > dmax_lout)
                    {
                        //dmax por corriente out
                        if (d > dmax_lout)
                            d = dmax_lout;
                    }
                    else
                    {
                        //dmax por vin
                        if (d > dmax_vin)
                            d = dmax_vin;
                    }
                    
                    if (d < 0)
                    {
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                    }

                    UpdateTIM_MosfetA(d);
                    
                }    //cierra undersampling
                
            }    //cierra sequence

            break;

        case MAIN_OVERVOLTAGE:
            if ((!timer_standby) && (sequence_ready))
            {
                sequence_ready_reset;
                if (Vout_Sense < VOUT_OVERVOLTAGE_THRESHOLD_TO_RECONNECT)
                {
                    main_state = MAIN_INIT;
                    Usart1Send((char *) "Reconnect...\n");
                }
            }
            break;

        case MAIN_UNDERVOLTAGE:
            if ((!timer_standby) && (sequence_ready))
            {
                sequence_ready_reset;
                if (vin_filtered > VIN_UNDERVOLTAGE_THRESHOLD_TO_RECONNECT)
                {
                    main_state = MAIN_INIT;
                    Usart1Send((char *) "Reconnect...\n");
                }
            }
            break;
            
        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    main_state = MAIN_JUMPER_PROTECT_OFF;
                    timer_standby = 400;
                }
            }                
            break;

        case MAIN_JUMPER_PROTECT_OFF:
            if (!timer_standby)
            {
                //vuelvo a INIT
                main_state = MAIN_INIT;
                Usart1Send((char *) "Protect OFF\n");                    
            }                
            break;
            
        case MAIN_OVERCURRENT:
            // if ((!PROT_MOS_A) && (!PROT_MOS_B))
            // {
            //     if ((!timer_standby) && (STOP_JUMPER))    //solo destrabo si se coloca el Jumper y se quita
            //     {                                         //en MAIN_JUMPER_PROTECTED
            //         LED_OFF;
            //         ENABLE_TIM3;
            //         ENABLE_TIM1;
            //         main_state = MAIN_JUMPER_PROTECTED;
            //     }
            // }
            break;

        default:
            main_state = MAIN_INIT;
            break;
        }	//fin switch main_state
        
        //Cosas que no tienen tanto que ver con las muestras o el estado del programa
        if ((STOP_JUMPER) &&
            (main_state != MAIN_JUMPER_PROTECTED) &&
            (main_state != MAIN_JUMPER_PROTECT_OFF) &&            
            (main_state != MAIN_OVERCURRENT))
        {
            UpdateTIM_MosfetA(DUTY_NONE);
            UpdateFB(DUTY_NONE);
            EXTIOff();
            main_state = MAIN_OVERVOLTAGE;
            Usart1Send((char *) "Protect ON\n");
            timer_standby = 1000;
            main_state = MAIN_JUMPER_PROTECTED;
        }

        //si esta corriendo reviso tension de salida y entrada
        if ((main_state == MAIN_SOFT_START) ||
            (main_state == MAIN_VOLTAGE_MODE))
        {
            //proteccion de sobretension
            if (Vout_Sense > VOUT_OVERVOLTAGE_THRESHOLD_TO_DISCONNECT)
            {
                UpdateTIM_MosfetA(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                EXTIOff();
#ifdef USE_LED_IN_PROT
                LED_ON;
#endif
                main_state = MAIN_OVERVOLTAGE;
                sprintf (s_lcd, "Overvoltage! Vout: %d d: %d\n", Vout_Sense, d);
                Usart1Send(s_lcd);
                
                timer_standby = 1000;
            }

            //proteccion de falta de tension
            if (vin_filtered < VIN_UNDERVOLTAGE_THRESHOLD_TO_DISCONNECT)            
            // if (Vin_Sense < VIN_UNDERVOLTAGE_THRESHOLD_TO_DISCONNECT)
            {
                UpdateTIM_MosfetA(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                EXTIOff();
#ifdef USE_LED_IN_PROT
                LED_ON;
#endif
                main_state = MAIN_UNDERVOLTAGE;
                // sprintf (s_lcd, "Undervoltage! VM: %d\n", Vin_Sense);
                sprintf (s_lcd, "Undervoltage! Vin: %d d: %d\n", vin_filtered, d);
                Usart1Send(s_lcd);

                timer_standby = 4000;                
            }
        }

        if (!timer_meas)
        {
            timer_meas = 2000;
            sprintf (s_lcd, "Vin: %d, Vout: %d, d: %d, dmax_vin: %d, dmax_lout: %d\n",
                     vin_filtered,
                     Vout_Sense,
                     d,
                     dmax_vin,
                     dmax_lout);
            
            Usart1Send(s_lcd);
        }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
            dmax_vin = UpdateDMAX(vin_filtered);
        }
    }
#endif    //USE_FORWARD_MODE

    
#endif    //ver 2.0
//---------- Fin VER_2_0 --------//

    return 0;
}

//--- End of Main ---//


void TimingDelay_Decrement(void)
{
    if (wait_ms_var)
        wait_ms_var--;

    if (timer_standby)
        timer_standby--;

    if (take_temp_sample)
        take_temp_sample--;

    if (timer_meas)
        timer_meas--;

    if (timer_led)
        timer_led--;

    if (timer_filters)
        timer_filters--;
    
    // //cuenta de a 1 minuto
    // if (secs > 59999)	//pasaron 1 min
    // {
    // 	minutes++;
    // 	secs = 0;
    // }
    // else
    // 	secs++;
    //
    // if (minutes > 60)
    // {
    // 	hours++;
    // 	minutes = 0;
    // }


}

//asi como esta tarda 800ns para ejecutar primera linea y 5.6us para la ultima linea
//TODO: cambiar todo lo que se pueda por MACROS
void EXTI4_15_IRQHandler(void)
{
#ifdef USE_FORWARD_MODE
    //actuando el timer 3 en el mosfet A
#ifdef USE_LED_IN_INT    
    LED_ON;
#endif    
    DisablePreload_MosfetA;
    UpdateTIM_MosfetA(0);
    EnablePreload_MosfetA;
    if (d > 10)
        d -= 10;
    UpdateTIM_MosfetA(d);
#ifdef USE_LED_IN_INT        
    LED_OFF;
#endif    
    EXTI->PR |= 0x00000010;    //4
#endif

#ifdef USE_PUSH_PULL_MODE
    //actuando el timer1 en mosfet B y timer3 en el mosfet A
#ifdef USE_LED_IN_INT    
    LED_ON;
#endif
    hard_overcurrent = 1;
    
#ifdef USE_LED_IN_INT        
    LED_OFF;
#endif    
    EXTI->PR |= 0x00000010;    //4
#endif
}


void SysTickError (void)
{
    //Capture systick error...
    while (1)
    {
        if (LED)
            LED_OFF;
        else
            LED_ON;

        for (unsigned char i = 0; i < 255; i++)
        {
            asm ("nop \n\t"
                 "nop \n\t"
                 "nop \n\t" );
        }
    }
}

//--- end of file ---//

