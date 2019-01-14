//-----------------------------------------------------
// #### PROYECTO MICROINVERSOR F030 - Custom Board ####
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    STM32F030
// ##
// #### MAIN.C ########################################
//-----------------------------------------------------

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gpio.h"
#include "tim.h"
#include "uart.h"
#include "hard.h"

#include "core_cm0.h"
#include "adc.h"
#include "dma.h"
#include "flash_program.h"
#include "dsp.h"

#include "it.h"



//--- VARIABLES EXTERNAS ---//


// ------- Externals del Puerto serie  -------
volatile unsigned char tx1buff[SIZEOF_DATA];
volatile unsigned char rx1buff[SIZEOF_DATA];
volatile unsigned char usart1_have_data = 0;

// ------- Externals del o para el ADC -------
volatile unsigned short adc_ch[ADC_CHANNEL_QUANTITY];
volatile unsigned char seq_ready = 0;


// ------- Externals para timers -------
volatile unsigned short timer_led = 0;


// ------- Externals para filtros -------
volatile unsigned short take_temp_sample = 0;

// ------- Definiciones para los filtros -------
#define SIZEOF_FILTER    8
#define UNDERSAMPLING_TICKS    10
unsigned short vin_vector [SIZEOF_FILTER];
// unsigned short vbatt [SIZEOF_FILTER];
// unsigned short iboost [SIZEOF_FILTER];


// parameters_typedef param_struct;

//--- VARIABLES GLOBALES ---//
volatile unsigned char current_excess = 0;
volatile short d = 0;
short ez1 = 0;
short ez2 = 0;
// unsigned short dmax = 0;
unsigned short last_d = 0;
#define DELTA_D    2

// ------- de los timers -------
volatile unsigned short wait_ms_var = 0;
volatile unsigned short timer_standby;
//volatile unsigned char display_timer;
volatile unsigned char timer_meas;
volatile unsigned char timer_filters = 0;

volatile unsigned short dmax_permited = 0;
volatile unsigned char llegue_tarde = 0;
// volatile unsigned short secs = 0;
// volatile unsigned char hours = 0;
// volatile unsigned char minutes = 0;






//--- FUNCIONES DEL MODULO ---//
void TimingDelay_Decrement (void);
void Overcurrent_Shutdown (void);

#ifdef VER_2_0
// ------- para el LM311 -------
extern void EXTI4_15_IRQHandler(void);
#endif


//--- Private Definitions ---//


//-------------------------------------------//
// @brief  Main program.
// @param  None
// @retval None
//------------------------------------------//
int main(void)
{
    unsigned char i;
    unsigned short ii;

    unsigned char undersampling = 0;
    main_state_t main_state = MAIN_INIT;
    unsigned short vin_filtered = 0;

// #ifdef TEST_FIXED_VOUT    
    unsigned short dmax_lout = 0;
    unsigned short dmax_vin = 0;    
    unsigned int delta_vout = 0;
    unsigned short normalized_vout = 0;
// #endif

#ifdef TEST_FIXED_D
    unsigned char stopped = 0;
#endif

#ifdef ONLY_COMMS
    unsigned short dmax_vin = 0;
#endif
    
    char s_lcd [120];		

    //GPIO Configuration.
    GPIO_Config();

    //ACTIVAR SYSTICK TIMER
    if (SysTick_Config(48000))
    {
        while (1)	/* Capture error */
        {
            if (LED)
                LED_OFF;
            else
                LED_ON;

            for (i = 0; i < 255; i++)
            {
                asm (	"nop \n\t"
                        "nop \n\t"
                        "nop \n\t" );
            }
        }
    }

    //--- Leo los parametros de memoria ---//

    // while (1)
    // {
    //  if (STOP_JUMPER)
    //  {
    //  	LED_OFF;
    //  }
    //  else
    //  {
    // 	  if (LED)
    // 	  	LED_OFF;
    // 	  else
    // 	  	LED_ON;
    //
    // 	  Wait_ms (250);
    //  }
    // }


//---------- Pruebas de Hardware --------//
    EXTIOff ();
    USART1Config();
    
    //---- Welcome Code ------------//
    //---- Defines from hard.h -----//
#ifdef HARD
    Usart1Send((char *) HARD);
    Wait_ms(100);
#else
#error	"No Hardware defined in hard.h file"
#endif

#ifdef SOFT
    Usart1Send((char *) SOFT);
    Wait_ms(100);
#else
#error	"No Soft Version defined in hard.h file"
#endif

#ifdef FEATURES
    WelcomeCodeFeatures(s_lcd);
#endif
    
//---------- Versiones 1_2 y 2_0  --------//    
#if (defined VER_2_0)
    
    TIM_1_Init ();	   //lo utilizo para mosfet Ctrol_M_B y para FB si esta definido en hard.h
    TIM_3_Init ();	   //lo utilizo para mosfet Ctrol_M_A y para synchro ADC

#ifdef WITH_TIM14_FB
    TIM_14_Init ();        //lo uso para FB
#endif
    

    AdcConfig();

    //-- DMA configuration.
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;

    //--- Prueba HARD pin FB ----------
    // //probar con WITH_TIM14_FB y WITH_TIM1_FB
    // while (1)
    // {
    //     for (ii = 0; ii < DUTY_100_PERCENT; ii++)
    //     {
    //         UpdateFB(ii);
    //         Wait_ms(1);
    //     }
    //     for (ii = DUTY_100_PERCENT; ii > 0; ii--)
    //     {
    //         UpdateFB(ii);
    //         Wait_ms(1);
    //     }
    // }
    //--- Fin Prueba HARD pin FB ----------

    //--- Prueba HARD pines CTRL_MOSFET ----------
    //pruebo seniales gate, el defasaje de las mismas y los flancos de I_Sense
    // UpdateTIMSync (DUTY_FOR_DMAX);
    // UpdateTIMSync (DUTY_10_PERCENT);    
    // while (1);
    //--- Fin Prueba HARD pines CTRL_MOSFET ----------

    //--- Prueba HARD pines ADC ----------
    // while (1)
    // {
    //     if (!timer_standby)
    //     {
    //         timer_standby = 2000;
    //         sprintf (s_lcd, "Vin: %d, Vout: %d, I: %d\n",
    //                  Vin_Sense,
    //                  Vout_Sense,
    //                  I_Sense);
            
    //         Usart1Send(s_lcd);
    //     }
    // }   
    //--- Fin Prueba HARD pines ADC ----------

    //--- Prueba tension de salida con max d fijo ----------
    //este loop trabaja en voltage-mode
    // while (1)
    // {
    //     if (sequence_ready)
    //     {
    //         sequence_ready_reset;
                
    //         if (undersampling < (UNDERSAMPLING_TICKS - 1))
    //         {
    //             undersampling++;
    //         }
    //         else
    //         {
    //             undersampling = 0;
    //             d = PID_roof (VOUT_110V, Vout_Sense, d, &ez1, &ez2);
                    
    //             if (d < 0)
    //             {
    //                 d = 0;
    //                 ez1 = 0;
    //                 ez2 = 0;
    //             }

    //             if (d > DUTY_5_PERCENT)
    //                 d = DUTY_5_PERCENT;

    //             UpdateTIMSync (d);
    //         }
    //     }

    //     if (!timer_standby)
    //     {
    //         timer_standby = 2000;
    //         sprintf (s_lcd, "Vin: %d, Vout: %d, I: %d, d: %d\n",
    //                  Vin_Sense,
    //                  Vout_Sense,
    //                  I_Sense,
    //                  d);
            
    //         Usart1Send(s_lcd);
    //     }

    // }
    //--- Fin Prueba tension de salida con max d fijo ----------
                
#ifdef USE_ONLY_VM_ONLY_MOSFET_A
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
                main_state = MAIN_VOLTAGE_MODE;
                LED_OFF;
            }

            if (sequence_ready)
                sequence_ready_reset;
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

                    //tambien por cada muestra calculo el dmax_permited
                    delta_vout = VinTicksToVoltage(Vin_Sense);                        
                    delta_vout = (delta_vout * N_TRAFO) / 1000;

                    normalized_vout = VoutTicksToVoltage(Vout_Sense);

                    if (delta_vout > normalized_vout)
                        delta_vout = delta_vout - normalized_vout;
                    else
                        delta_vout = 0;

                    dmax_lout = UpdateDmaxLout((unsigned short)delta_vout);

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
                sprintf (s_lcd, "Overvoltage! VM: %d\n", Vout_Sense);
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
                sprintf (s_lcd, "Undervoltage! VM: %d\n", vin_filtered);                
                Usart1Send(s_lcd);

                timer_standby = 4000;                
            }
            
            //se deshabilito la int, espero que se libere la pata
            // if ((llegue_tarde) && (!PROT_MOS))
            // {
            //     llegue_tarde = 0;
            //     EXTIOn();
            //     //MosfetA por ser One Pulse Mode, necesita ser modifcado ahora
            //     DisablePreload_MosfetA;
            //     UpdateTIMSync(dmax_permited);
            //     EnablePreload_MosfetA;
            // }
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
                    //vuelvo a INIT
                    main_state = MAIN_INIT;
                    Usart1Send((char *) "Protect OFF\n");                    
                }
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

        if (!timer_standby)
        {
            timer_standby = 2000;
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
#endif    //USE_ONLY_VM_ONLY_MOSFET_A

#ifdef USE_ONLY_CM_ONLY_MOSFET_A
    //uso solo mosfet de TIM3, mosfet A
    timer_standby = 2000;
    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (!timer_standby)
            {
                EnablePreload_MosfetA;
                EXTIOn();
                //si no le pongo esto puede que no arranque
                dmax_permited = 450;
                UpdateTIM_MosfetA(dmax_permited);
                UpdateFB(DUTY_NONE);
                main_state = MAIN_CURRENT_MODE;
                timer_standby = 2000;
            }

            if (sequence_ready)
                sequence_ready_reset;
            break;

        case MAIN_CURRENT_MODE:
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
                        //me pase de tension voy a voltage mode para tener buena regulacion en vacio
                        UpdateFB(DUTY_NONE);
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                    }
                    else
                    {
                        //seteo FB y espero int del lazo de corriente
                        if (d > DUTY_FB_25A)
                            d = DUTY_FB_25A;

                        //por ahora no seteo d
                        UpdateFB(d);
                    }
                }
                
                //tambien por cada muestra calculo el dmax_permited
                delta_vout = VinTicksToVoltage(Vin_Sense);                        
                delta_vout = (delta_vout * N_TRAFO) / 1000;

                normalized_vout = VoutTicksToVoltage(Vout_Sense);

                if (delta_vout > normalized_vout)
                    delta_vout = delta_vout - normalized_vout;
                else
                    delta_vout = 0;

                dmax_lout = UpdateDmaxLout((unsigned short)delta_vout);
                // if (dmax_vin > dmax_lout)
                // {
                //     //dmax por corriente out
                //     EXTIOff();
                //     dmax_permited = dmax_lout;
                //     EXTIOn();
                // }
                // else
                // {
                //     //dmax por vin
                //     EXTIOff();
                //     dmax_permited = dmax_vin;
                //     EXTIOn();
                // }
                //fin calculo dmax_permited
                
            }    //cierra sequence

            //proteccion de sobretension
            if (Vout_Sense > VOUT_OVERVOLTAGE_THRESHOLD_TO_DISCONNECT)
            {
                UpdateTIM_MosfetA(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                EXTIOff();
                LED_ON;
                main_state = MAIN_OVERVOLTAGE;
                Usart1Send((char *) "Overvoltage! CM\n");
            }

            //se deshabilito la int, espero que se libere la pata
            // if ((llegue_tarde) && (!PROT_MOS))
            // {
            //     llegue_tarde = 0;
            //     EXTIOn();
            //     //MosfetA por ser One Pulse Mode, necesita ser modifcado ahora
            //     DisablePreload_MosfetA;
            //     UpdateTIMSync(dmax_permited);
            //     EnablePreload_MosfetA;
            // }
            break;

        case MAIN_OVERVOLTAGE:
            if ((!timer_standby) && (sequence_ready))
            {
                sequence_ready_reset;
                if (Vout_Sense < VOUT_OVERVOLTAGE_THRESHOLD_TO_RECONNECT)
                {
                    LED_ON;
                    timer_standby = 2000;
                    // main_state = MAIN_SOFT_START;
                    Usart1Send((char *) "Reconnect...\n");
                }
            }
            break;
            
        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    //vuelvo a INIT
                    main_state = MAIN_INIT;
                }
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
            (main_state != MAIN_OVERCURRENT))
        {
            UpdateTIMSync (0);
            d = 0;
            last_d = 0;
            timer_standby = 300;    //doy minimo 300ms para reactivar
            main_state = MAIN_JUMPER_PROTECTED;
        }

        if (!timer_standby)
        {
            timer_standby = 2000;
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
#endif    //USE_ONLY_CM_ONLY_MOSFET_B
    
#ifdef USE_ONLY_CM_ONLY_MOSFET_B
    //uso solo mosfet de TIM1, mosfet B
    timer_standby = 2000;
    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (!timer_standby)
            {
                EnablePreload_MosfetB;
                EXTIOn();
                //si no le pongo esto puede que no arranque
                dmax_permited = 150;
                UpdateTIM_MosfetB(dmax_permited);
                UpdateFB(DUTY_NONE);
                main_state = MAIN_CURRENT_MODE;
                timer_standby = 2000;
            }

            if (sequence_ready)
                sequence_ready_reset;
            break;

        case MAIN_CURRENT_MODE:
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
                        //me pase de tension voy a voltage mode para tener buena regulacion en vacio
                        UpdateFB(DUTY_NONE);
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                    }
                    else
                    {
                        //seteo FB y espero int del lazo de corriente
                        if (d > DUTY_100_PERCENT)
                            d = DUTY_100_PERCENT;

                        //por ahora no seteo d
                        // UpdateFB(d);
                    }
                }
                
                //tambien por cada muestra calculo el dmax_permited
                delta_vout = VinTicksToVoltage(Vin_Sense);                        
                delta_vout = (delta_vout * N_TRAFO) / 1000;

                normalized_vout = VoutTicksToVoltage(Vout_Sense);

                if (delta_vout > normalized_vout)
                    delta_vout = delta_vout - normalized_vout;
                else
                    delta_vout = 0;

                dmax_lout = UpdateDmaxLout((unsigned short)delta_vout);
                // if (dmax_vin > dmax_lout)
                // {
                //     //dmax por corriente out
                //     EXTIOff();
                //     dmax_permited = dmax_lout;
                //     EXTIOn();
                // }
                // else
                // {
                //     //dmax por vin
                //     EXTIOff();
                //     dmax_permited = dmax_vin;
                //     EXTIOn();
                // }
                //fin calculo dmax_permited
                
            }    //cierra sequence

            //proteccion de sobretension
            if (Vout_Sense > VOUT_OVERVOLTAGE_THRESHOLD_TO_DISCONNECT)
            {
                UpdateTIM_MosfetB(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                EXTIOff();
                LED_ON;
                main_state = MAIN_OVERVOLTAGE;
                Usart1Send((char *) "Overvoltage! CM\n");
            }

            //se deshabilito la int, espero que se libere la pata
            // if ((llegue_tarde) && (!PROT_MOS))
            // {
            //     llegue_tarde = 0;
            //     EXTIOn();
            //     //MosfetA por ser One Pulse Mode, necesita ser modifcado ahora
            //     DisablePreload_MosfetA;
            //     UpdateTIMSync(dmax_permited);
            //     EnablePreload_MosfetA;
            // }
            break;

        case MAIN_OVERVOLTAGE:
            if ((!timer_standby) && (sequence_ready))
            {
                sequence_ready_reset;
                if (Vout_Sense < VOUT_OVERVOLTAGE_THRESHOLD_TO_RECONNECT)
                {
                    LED_ON;
                    timer_standby = 2000;
                    // main_state = MAIN_SOFT_START;
                    Usart1Send((char *) "Reconnect...\n");
                }
            }
            break;
            
        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    //vuelvo a INIT
                    main_state = MAIN_INIT;
                }
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
            (main_state != MAIN_OVERCURRENT))
        {
            UpdateTIMSync (0);
            d = 0;
            last_d = 0;
            timer_standby = 300;    //doy minimo 300ms para reactivar
            main_state = MAIN_JUMPER_PROTECTED;
        }

        if (!timer_standby)
        {
            timer_standby = 2000;
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
#endif    //USE_ONLY_CM_ONLY_MOSFET_B

#ifdef USE_ONLY_CM
    timer_standby = 2000;
    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (!timer_standby)
            {
                EnablePreload_MosfetA;
                EnablePreload_MosfetB;
                EXTIOn();
                //si no le pongo esto puede que no arranque
                dmax_permited = 25;
                UpdateTIMSync(dmax_permited);
                // UpdateTIMSync(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                main_state = MAIN_CURRENT_MODE;
                timer_standby = 2000;
            }

            if (sequence_ready)
                sequence_ready_reset;
            break;

        case MAIN_CURRENT_MODE:
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
                        //me pase de tension voy a voltage mode para tener buena regulacion en vacio
                        UpdateFB(DUTY_NONE);
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                    }
                    else
                    {
                        //seteo FB y espero int del lazo de corriente
                        if (d > DUTY_100_PERCENT)
                            d = DUTY_100_PERCENT;

                        //por ahora no seteo d
                        // UpdateFB(d);
                    }
                }
                
                //tambien por cada muestra calculo el dmax_permited
                delta_vout = VinTicksToVoltage(Vin_Sense);                        
                delta_vout = (delta_vout * N_TRAFO) / 1000;

                normalized_vout = VoutTicksToVoltage(Vout_Sense);

                if (delta_vout > normalized_vout)
                    delta_vout = delta_vout - normalized_vout;
                else
                    delta_vout = 0;

                dmax_lout = UpdateDmaxLout((unsigned short)delta_vout);
                if (dmax_vin > dmax_lout)
                {
                    //dmax por corriente out
                    EXTIOff();
                    dmax_permited = dmax_lout;
                    EXTIOn();
                }
                else
                {
                    //dmax por vin
                    EXTIOff();
                    dmax_permited = dmax_vin;
                    EXTIOn();
                }
                //fin calculo dmax_permited
                
            }    //cierra sequence

            //proteccion de sobretension
            if (Vout_Sense > VOUT_OVERVOLTAGE_THRESHOLD_TO_DISCONNECT)
            {
                UpdateTIMSync(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                EXTIOff();
                LED_ON;
                main_state = MAIN_OVERVOLTAGE;
                Usart1Send((char *) "Overvoltage! CM\n");
            }

            //se deshabilito la int, espero que se libere la pata
            // if ((llegue_tarde) && (!PROT_MOS))
            // {
            //     llegue_tarde = 0;
            //     EXTIOn();
            //     //MosfetA por ser One Pulse Mode, necesita ser modifcado ahora
            //     DisablePreload_MosfetA;
            //     UpdateTIMSync(dmax_permited);
            //     EnablePreload_MosfetA;
            // }
            break;

        case MAIN_OVERVOLTAGE:
            if ((!timer_standby) && (sequence_ready))
            {
                sequence_ready_reset;
                if (Vout_Sense < VOUT_OVERVOLTAGE_THRESHOLD_TO_RECONNECT)
                {
                    LED_ON;
                    timer_standby = 2000;
                    // main_state = MAIN_SOFT_START;
                    Usart1Send((char *) "Reconnect...\n");
                }
            }
            break;
            
        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    //vuelvo a INIT
                    main_state = MAIN_INIT;
                }
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
            (main_state != MAIN_OVERCURRENT))
        {
            UpdateTIMSync (0);
            d = 0;
            last_d = 0;
            timer_standby = 300;    //doy minimo 300ms para reactivar
            main_state = MAIN_JUMPER_PROTECTED;
        }

        if (!timer_standby)
        {
            timer_standby = 2000;
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
#endif    //USE_ONLY_CM
    
#ifdef USE_VM_AND_CM
    //programa de produccion tiene dos loops uno voltage mode y el otro current mode
    //ademas revisa y hace un update del din_max para no saturar al trafo
    timer_standby = 2000;
    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (!timer_standby)
            {
                // EXTIOn();
                // UpdateTIMSync(dmax_vin);
                UpdateTIMSync(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                main_state = MAIN_SOFT_START;
                timer_standby = 2000;
            }

            if (sequence_ready)
                sequence_ready_reset;
            break;

        case MAIN_SOFT_START:
        case MAIN_VOLTAGE_MODE:
            //lazo voltage mode con dmax_vin o dmax_lout como tope
            if (sequence_ready)
            {
                sequence_ready_reset;

                if (undersampling < (UNDERSAMPLING_TICKS - 1))
                {                    
                    if (undersampling == (UNDERSAMPLING_TICKS - 2))
                    {
                        delta_vout = VinTicksToVoltage(Vin_Sense);                        
                        delta_vout = (delta_vout * N_TRAFO) / 1000;

                        normalized_vout = VoutTicksToVoltage(Vout_Sense);

                        if (delta_vout > normalized_vout)
                            delta_vout = delta_vout - normalized_vout;
                        else
                            delta_vout = 0;

                        dmax_lout = UpdateDmaxLout((unsigned short)delta_vout);
                    }
                    undersampling++;
                }
                else
                {
                    undersampling = 0;
                    // d = PID_roof (VOUT_350V, Vout_Sense, d, &ez1, &ez2);
                    // d = PID_roof (VOUT_300V, Vout_Sense, d, &ez1, &ez2);                    
                    d = PID_roof (VOUT_SETPOINT, Vout_Sense, d, &ez1, &ez2);                    
                    
                    if (d < 0)
                    {
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                    }

                    //maximos del pwm por corriente en bobina de salida
                    //o saturacion de trafo por tension de entrada
                    if (dmax_vin > dmax_lout)
                    {
                        //dmax por corriente out
                        if (d > dmax_lout)
                            d = dmax_lout;
                        
                        UpdateTIMSync(d);
                    }
                    else
                    {
                        //dmax por vin
                        if (d > dmax_vin)
                            d = dmax_vin;
                        
                        UpdateTIMSync(d);
                    }

                    //reviso el cambio de modo
                    if ((Vout_Sense < VOUT_HIGH_MODE_CHANGE) &&
                        (Vout_Sense > VOUT_LOW_MODE_CHANGE) &&
                        (d > DUTY_TO_CHANGE_CURRENT_MODE) &&
                        (!timer_standby))
                    {
                        UpdateTIMSync(dmax_vin);
                        llegue_tarde = 0;
                        main_state = MAIN_CURRENT_MODE;
                        Usart1Send((char *) "To CM\n");
                        EXTIOn();
                    }
                }
            }    //cierra sequence

            //proteccion de sobretension
            if (Vout_Sense > VOUT_OVERVOLTAGE_THRESHOLD_TO_DISCONNECT)
            {
                UpdateTIMSync(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                LED_ON;
                main_state = MAIN_OVERVOLTAGE;
                Usart1Send((char *) "Overvoltage! VM\n");
            }
            break;

        case MAIN_CURRENT_MODE:
            if (sequence_ready)
            {
                sequence_ready_reset;

                if (undersampling < (UNDERSAMPLING_TICKS - 1))
                    undersampling++;
                else
                {
                    undersampling = 0;
                    d = PID_roof (VOUT_SETPOINT, Vout_Sense, d, &ez1, &ez2);                    
                    
                    if (d < DUTY_TO_CHANGE_VOLTAGE_MODE)
                    {
                        //me pase de tension voy a voltage mode para tener buena regulacion en vacio
                        main_state = MAIN_VOLTAGE_MODE;
                        UpdateFB(DUTY_NONE);
                        EXTIOff();
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                        Usart1Send((char *) "To VM\n");
                    }
                    else
                    {
                        //seteo FB y espero int del lazo de corriente
                        if (d > DUTY_100_PERCENT)
                            d = DUTY_100_PERCENT;
                        
                        UpdateFB(d);
                    }
                }
            }    //cierra sequence

            //proteccion de sobretension
            if (Vout_Sense > VOUT_OVERVOLTAGE_THRESHOLD_TO_DISCONNECT)
            {
                UpdateTIMSync(DUTY_NONE);
                UpdateFB(DUTY_NONE);
                EXTIOff();
                LED_ON;
                main_state = MAIN_OVERVOLTAGE;
                Usart1Send((char *) "Overvoltage! CM\n");
            }

            if (llegue_tarde == 1)
            {
                Usart1Send((char *) "Tarde!!!\n");
                llegue_tarde = 2;
            }
            break;

        case MAIN_OVERVOLTAGE:
            if (sequence_ready)
            {
                sequence_ready_reset;
                if (Vout_Sense < VOUT_OVERVOLTAGE_THRESHOLD_TO_RECONNECT)
                {
                    LED_ON;
                    main_state = MAIN_SOFT_START;
                    Usart1Send((char *) "Reconnect...\n");
                }
            }
            break;
            
        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    //vuelvo a INIT
                    main_state = MAIN_INIT;
                }
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
            (main_state != MAIN_OVERCURRENT))
        {
            UpdateTIMSync (0);
            d = 0;
            last_d = 0;
            timer_standby = 300;    //doy minimo 300ms para reactivar
            main_state = MAIN_JUMPER_PROTECTED;
        }

        if (!timer_standby)
        {
            timer_standby = 2000;
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
    }    //end of while 1
#endif    //USE_VM_AND_CM
    
#endif    //current mode ver 2.0
//---------- Fin Test CURRENT_MODE_VER_2_0 --------//

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
#ifdef USE_ONLY_VM_ONLY_MOSFET_A
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

#ifdef USE_ONLY_CM_ONLY_MOSFET_A
    //actuando el timer 3 en el mosfet A
    LED_ON;
    DisablePreload_MosfetA;
    UpdateTIM_MosfetA(0);
    EnablePreload_MosfetA;
    UpdateTIM_MosfetA(dmax_permited);

    LED_OFF;
    EXTI->PR |= 0x00000010;    //4
#endif
    
#ifdef USE_ONLY_CM_ONLY_MOSFET_B
    //actuando el timer 1 en el mosfet B
    LED_ON;
    DisablePreload_MosfetB;
    UpdateTIM_MosfetB(0);
    EnablePreload_MosfetB;
    UpdateTIM_MosfetB(dmax_permited);        

    LED_OFF;
    EXTI->PR |= 0x00000010;    //4
#endif
    
#ifdef USE_ONLY_CM
    //el timer 1 dispara al timer 3
    if (TIM1->CNT < DUTY_50_PERCENT)
    {
        //actuando el timer 1 en el mosfet B
        LED_ON;
        DisablePreload_MosfetB;
        UpdateTIM_MosfetB(0);
        EnablePreload_MosfetB;
        UpdateTIM_MosfetB(dmax_permited);        
    }
    else   
    {
        //actuando el timer 3 en el mosfet A
        LED_ON;
        DisablePreload_MosfetA;
        UpdateTIM_MosfetA(0);
        EnablePreload_MosfetA;
        UpdateTIM_MosfetA(dmax_permited);        
    }
    // if (SENSE_MOSFET_A)
    // {
    //     LED_ON;
    //     DisablePreload_MosfetA;
    //     UpdateTIM_MosfetA(0);
    //     EnablePreload_MosfetA;
    //     UpdateTIM_MosfetA(dmax_permited);
    // }
    // else if (SENSE_MOSFET_B)
    // {
    //     LED_ON;
    //     DisablePreload_MosfetB;
    //     UpdateTIM_MosfetB(0);
    //     EnablePreload_MosfetB;
    //     UpdateTIM_MosfetB(dmax_permited);
    // }
    // else
    // {
    //     //llegue tarde o hay ruido; paro los dos mosfets y luego los arranco
    //     //paro mosfet A
    //     DisablePreload_MosfetA;
    //     UpdateTIM_MosfetA(0);

    //     //paro mosfet B
    //     DisablePreload_MosfetB;
    //     UpdateTIM_MosfetB(0);

    //     EnablePreload_MosfetA;
    //     UpdateTIM_MosfetA(dmax_permited);
        
    //     EnablePreload_MosfetB;
    //     UpdateTIM_MosfetB(dmax_permited);
    // }
    LED_OFF;
    EXTI->PR |= 0x00000010;    //4
#endif
}

//------ EOF -------//
