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
short d = 0;
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

#if defined VER_1_1 || defined VER_1_2
// ------- para el LM393N -------
extern void EXTI4_15_IRQHandler(void);
#endif

#ifdef VER_1_0
// ------- para el LM311 -------
extern void EXTI0_1_IRQHandler(void);
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
    
//---------- Test CURRENT_MODE_VER_1_2 --------//    
#if (defined CURRENT_MODE_VER_1_2) || (defined CURRENT_MODE_VER_2_0)
    
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
    
#endif    //current mode ver 1.2 or current mode ver 2.0
//---------- Fin Test CURRENT_MODe_VER_1_2 --------//

//---------- Test CURRENT_MODE_VER_1_0 --------//    
#ifdef CURRENT_MODE_VER_1_0

    TIM_1_Init ();					//lo utilizo para mosfet Ctrol_M_B, y pin FB
    TIM_3_Init ();					//lo utilizo para mosfet Ctrol_M_A y para synchro ADC

    AdcConfig();

    //-- DMA configuration.
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;

    while (1)
    {
        if (FB)
            FB_OFF;
        else
            FB_ON;
        Wait_ms (100);
    }
    
    while (1)
    {
        for (ii = 0; ii < DUTY_100_PERCENT; ii++)
        {
            UpdateFB(ii);
            Wait_ms(1);
        }
        for (ii = DUTY_100_PERCENT; ii > 0; ii--)
        {
            UpdateFB(ii);
            Wait_ms(1);
        }
    }
    
    //este programa tiene dos loops uno de hardware con la corriente pico
    //otro de soft con la tension de salida
    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (sequence_ready)
            {
                sequence_ready_reset;
                EXTIOn();
                UpdateTIMSync(DUTY_FOR_DMAX);
                UpdateFB(DUTY_10_PERCENT);
                main_state = MAIN_SOFT_START;
            }
            break;

        case MAIN_SOFT_START:
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
                    d = PID_roof (VOUT_350V, Vout_Sense, d, &ez1, &ez2);
                    // d = PID_roof (VOUT_300V, Vout_Sense, d, &ez1, &ez2);                    
                    // d = PID_roof (VOUT_200V, Vout_Sense, d, &ez1, &ez2);                    
                    
                    if (d < 0)
                    {
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                    }

                    if (dmax_vin > dmax_lout)
                    {
                        if (d > dmax_lout)
                        {
                            d = dmax_lout;
                            ez1 = 0;
                            ez2 = 0;
                        }
                    }
                    else
                    {
                        if (d > dmax_vin)
                        {
                            d = dmax_vin;
                            ez1 = 0;
                            ez2 = 0;
                        }
                    }

                    //derivativo exterior DELTA solo cuando incrementa
                    if (d > (last_d + DELTA_D))
                    {
                        d = last_d + DELTA_D;
                        last_d = d;
                        ez1 = 0;
                        ez2 = 0;
                    }
                    else
                        last_d = d;
                                                
                    UpdateTIMSync (d);

                    if (Vout_Sense > VOUT_SOFT_START)
                        main_state = MAIN_GENERATING;
                }
            }    //cierra sequence
            break;

        case MAIN_GENERATING:
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
        
        if (sequence_ready)
            sequence_ready_reset;
        
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

        // if (!timer_standby)
        // {
        //     timer_standby = 2000;
        //     sprintf (s_lcd, "Vin: %d, Vout: %d, d: %d, dmax_vin: %d\n",
        //              vin_filtered,
        //              Vout_Sense,
        //              d, dmax_vin);
            
        //     Usart1Send(s_lcd);
        // }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
            // dmax_vin = UpdateDMAX(vin_filtered);
        }
    }    //end of while 1
#endif    //current mode
//---------- Fin Test CURRENT_MODe_VER_1_0 --------//
    
//---------- Test ONLY_COMMS VER_1_0 --------//    
#ifdef ONLY_COMMS

    TIM_1_Init ();					//lo utilizo para mosfet Ctrol_M_B,
    TIM_3_Init ();					//lo utilizo para mosfet Ctrol_M_A y para synchro ADC

    AdcConfig();

    //-- DMA configuration.
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;


    while (1)
    {
        if (sequence_ready)
            sequence_ready_reset;
        
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
            sprintf (s_lcd, "Vin: %d, Vout: %d, d: %d, dmax_vin: %d\n",
                     vin_filtered,
                     Vout_Sense,
                     d, dmax_vin);
            
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
#endif
//---------- Fin Test ONLY_COMMS VER_1_0 --------//
    
//---------- Test FIXED VOUT VER_1_1 --------//
    //poner y quitar el jumper me recupera del overcurrent
#ifdef TEST_FIXED_VOUT

    TIM_1_Init ();					//lo utilizo para mosfet Ctrol_M_B,
    TIM_3_Init ();					//lo utilizo para mosfet Ctrol_M_A y para synchro ADC

    AdcConfig();

    //-- DMA configuration.
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;

    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (sequence_ready)
            {
                sequence_ready_reset;
                if (LED)
                    LED_OFF;
                else
                    LED_ON;

                UpdateTIMSync(0);
                d = 0;
                last_d = 0;
                // dmax = 0;
                dmax_vin = 0;
                dmax_lout = 0;
                ez1 = 0;
                ez2 = 0;
                EXTIOn();
                main_state = MAIN_SOFT_START;
            }
            break;

        case MAIN_SOFT_START:
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

                        
                        // delta_vout = MAX_VOUT - Vout_Sense;
                        // delta_vout = VoutTicksToVoltage (delta_vout);
                        // dmax_lout = UpdateDmaxLout (delta_vout);                        
                    }
                    
                    undersampling++;
                }
                else
                {
                    undersampling = 0;
                    d = PID_roof (VOUT_350V, Vout_Sense, d, &ez1, &ez2);
                    // d = PID_roof (VOUT_300V, Vout_Sense, d, &ez1, &ez2);                    
                    // d = PID_roof (VOUT_200V, Vout_Sense, d, &ez1, &ez2);                    
                    
                    if (d < 0)
                    {
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                    }

                    if (dmax_vin > dmax_lout)
                    {
                        if (d > dmax_lout)
                        {
                            d = dmax_lout;
                            ez1 = 0;
                            ez2 = 0;
                        }
                    }
                    else
                    {
                        if (d > dmax_vin)
                        {
                            d = dmax_vin;
                            ez1 = 0;
                            ez2 = 0;
                        }
                    }

                    //derivativo exterior DELTA solo cuando incrementa
                    if (d > (last_d + DELTA_D))
                    {
                        d = last_d + DELTA_D;
                        last_d = d;
                        ez1 = 0;
                        ez2 = 0;
                    }
                    else
                        last_d = d;
                                                
                    UpdateTIMSync (d);

                    if (Vout_Sense > VOUT_SOFT_START)
                        main_state = MAIN_GENERATING;
                }
            }    //cierra sequence
            break;

        case MAIN_GENERATING:
            if (sequence_ready)
            {
                sequence_ready_reset;
                
                if (undersampling < (UNDERSAMPLING_TICKS - 1))
                {
                    undersampling++;
                }
                else
                {
                    undersampling = 0;
                    d = PID_roof (VOUT_350V, Vout_Sense, d, &ez1, &ez2);
                    
                    if (d < 0)
                    {
                        d = 0;
                        ez1 = 0;
                        ez2 = 0;
                    }

                    if (d > dmax_vin)
                    {
                        d = dmax_vin;
                    }

                    //derivativo exterior DELTA solo cuando incrementa
                    if (d > (last_d + DELTA_D))
                    {
                        d = last_d + DELTA_D;
                        last_d = d;
                        ez1 = 0;
                        ez2 = 0;
                    }
                    else
                        last_d = d;

                    UpdateTIMSync (d);
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
            if ((!PROT_MOS_A) && (!PROT_MOS_B))
            {
                if ((!timer_standby) && (STOP_JUMPER))    //solo destrabo si se coloca el Jumper y se quita
                {                                         //en MAIN_JUMPER_PROTECTED
                    LED_OFF;
                    ENABLE_TIM3;
                    ENABLE_TIM1;
                    main_state = MAIN_JUMPER_PROTECTED;
                }
            }
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
                     d, dmax_vin, dmax_lout);
            
            Usart1Send(s_lcd);
        }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
            // if (main_state == MAIN_SOFT_START)
            //     dmax_vin = UpdateDMAXSF(vin_filtered);
            // else
                dmax_vin = UpdateDMAX(vin_filtered);
        }

        if (current_excess)
        {
            if (current_excess == 4)
                Usart1Send("\n Overcurrent on Q2 MOS_A!\n");
            else if (current_excess == 5)
                Usart1Send("\n Overcurrent on Q3 MOS_B!\n");
            else
                Usart1Send("\n Overcurrent!\n");

            main_state = MAIN_OVERCURRENT;
            timer_standby = 500;
            current_excess = 0;            
        }
    }       
#endif    //TEST_FIXED_VOUT
//---------- Fin Test FIXED VOUT VER_1_1 --------//    

//---------- Test FIXED D VER_1_1 --------//
#ifdef TEST_FIXED_D

    TIM_1_Init ();					//lo utilizo para mosfet Ctrol_M_B,
    TIM_3_Init ();					//lo utilizo para mosfet Ctrol_M_A y para synchro ADC

    AdcConfig();

    //-- DMA configuration.
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;
    EXTIOn();
    // UpdateTIMSync (25);

    while (1)
    {
        if (sequence_ready)
        {
            sequence_ready_reset;
            if (LED)
                LED_OFF;
            else
                LED_ON;
        }

        if (STOP_JUMPER)
        {
            if (!stopped)
            {
                UpdateTIMSync (0);
                d = 0;
                stopped = 1;
                Usart1Send("Stopped!\n");
                timer_standby = 1000;
            }
        }
        else
        {
            if ((stopped) && (!timer_standby))
            {
                Usart1Send("Starting...\n");
                stopped = 0;
                d = D_FOR_FIXED;
                UpdateTIMSync (d);
            }
        }

        if (!timer_standby)
        {
            timer_standby = 2000;
            sprintf (s_lcd, "Vin: %d, Vout: %d, d: %d\n", vin_filtered, Vout_Sense, d);
            Usart1Send(s_lcd);
        }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
        }

        if (current_excess)
        {
            if (current_excess == 4)
                Usart1Send("\n Overcurrent on Q2 MOS_A!\n");
            else if (current_excess == 5)
                Usart1Send("\n Overcurrent on Q3 MOS_B!\n");
            else
                Usart1Send("\n Overcurrent!\n");

            current_excess = 0;            
        }
    }       
#endif    //TEST_FIXED_D
//---------- Fin Test FIXED D VER_1_1 --------//    
    
//---------- Test ADC -> DMA VER_1_1 --------//
#ifdef TEST_ADC_AND_DMA

    TIM_1_Init ();					//lo utilizo para mosfet Ctrol_M_B,
    TIM_3_Init ();					//lo utilizo para mosfet Ctrol_M_A y para synchro ADC

    AdcConfig();

    //-- DMA configuration.
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;
    UpdateTIMSync (10);    
    
    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (sequence_ready)
            {
                sequence_ready_reset;
                if (LED)
                    LED_OFF;
                else
                    LED_ON;
            }
            break;
        }

        if (!timer_standby)
        {
            timer_standby = 2000;
            sprintf (s_lcd, "VIN: %d, VOUT: %d, dmax: %d\n", vin_filtered, Vout_Sense, dmax);
            Usart1Send(s_lcd);
        }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
            dmax = UpdateDMAX(vin_filtered);
        }
        
    }
    
#endif    //TEST_ADC_AND_DMA
//---------- Fin Test ADC -> DMA VER_1_1 --------//    

//---------- Test INT VER_1_1 --------//    
#ifdef TEST_INT_PRGRM
    //arranca como programa de produccion pero no mueve led, solo lo prende en INT
    //RECORDAR QUITAR JUMPER en driver (para no mover mosfets)
    //colocar generador de funciones en I_MOS_A o I_MOS_B senial triangular
    TIM_1_Init ();					//lo utilizo para mosfet Ctrol_M_B,
    TIM_3_Init ();					//lo utilizo para mosfet Ctrol_M_A y para synchro ADC

    AdcConfig();

    //-- DMA configuration.
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;
    UpdateTIMSync (25);
    
    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            if (sequence_ready)
            {
                sequence_ready_reset;
                if (LED)
                    LED_OFF;
                else
                    LED_ON;
            }
            break;

        case MAIN_SYNCHRO_ADC:
            // if (seq_ready)
            // {
                Usart1Send((char *) (const char *) "ADC Sync getted!\r\n");
                main_state = MAIN_SET_ZERO_CURRENT;
                seq_ready = 0;
                timer_standby = 2000;
            // }
            break;

        case MAIN_SET_ZERO_CURRENT:
            if (!timer_standby)
            {
                //a esta altura debo tener bien medida la tension de alimentacion para poder determina dmax
                d = 0;
                main_state = MAIN_GENERATING;
                EXTIOn();
            }                
            break;

        case MAIN_GENERATING:
            if (!STOP_JUMPER)
            {
                if (seq_ready)
                {
                    seq_ready = 0;

                    if (undersampling < UNDERSAMPLING_TICKS)
                        undersampling++;
                    else
                    {
                        d = PID_roof (VOUT_200V, Vout_Sense, d, &ez1, &ez2);
                    
                        if (d < 0)
                            d = 0;

                        // if (d > dmax)
                        //     d = dmax;

                        if (d > 100)    //traba de hardware
                            d = 100;
                        
                        UpdateTIMSync (d);
                        dmax = UpdateDMAX(vin_filtered);    //TODO: luego meter el filtro en sync con muestras
                    }
                }    //cierra sequence
            }
            else
            {
                UpdateTIMSync (0);
                d = 0;
                timer_standby = 300;    //doy minimo 300ms para reactivar
                main_state = MAIN_JUMPER_PROTECTED;
            }            
            break;

        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    //vuelvo a INIT
                    main_state = MAIN_SET_ZERO_CURRENT;
                }
            }                
            break;

        case MAIN_OVERCURRENT:
            if ((!PROT_MOS_A) && (!PROT_MOS_B))
            {
                LED_OFF;
                ENABLE_TIM3;
                ENABLE_TIM1;
                seq_ready = 0;
                // main_state = MAIN_SYNCHRO_ADC;
                main_state = MAIN_GENERATING;
            }
            break;

        default:
            seq_ready = 0;
            main_state = MAIN_SYNCHRO_ADC;
            break;
        }	//fin switch main_state

        if (!timer_standby)
        {
            timer_standby = 2000;
            sprintf (s_lcd, "VIN: %d, VOUT: %d, d: %d\r\n", Vin_Sense, Vout_Sense, d);
            Usart1Send(s_lcd);
        }

        if (current_excess)
        {
            current_excess = 0;
            Usart1Send("\r\n Overcurrent!");
            main_state = MAIN_OVERCURRENT;
        }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
        }
                        
        // UpdateLed();
        
    }	//fin while 1
#endif //TEST_INT_PRGRM


//---------- Programa de Produccion --------//    
    //--- Welcome code ---//
#ifdef PRODUCTION_PRGRM
    AdcConfig();		//recordar habilitar sensor en adc.h

    TIM_1_Init ();					//lo utilizo para mosfet Ctrol_M_B,
    TIM_3_Init ();					//lo utilizo para mosfet Ctrol_M_A y para synchro ADC
#ifdef VER_1_0
    TIM_14_Init();					//Set current overflow
#endif

    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            main_state = MAIN_SYNCHRO_ADC;
            ADC1->CR |= ADC_CR_ADSTART;
            seq_ready = 0;

#ifdef VER_1_0
            Update_TIM14_CH1 (512);		//permito 1.75V en LM311
#endif
            ChangeLed(LED_STANDBY);
            break;

        case MAIN_SYNCHRO_ADC:
            if (seq_ready)
            {
                Usart1Send((char *) (const char *) "ADC Sync getted!\r\n");
                main_state = MAIN_SET_ZERO_CURRENT;
                seq_ready = 0;
                timer_standby = 2000;                
            }
            break;

        case MAIN_SET_ZERO_CURRENT:
            if (!timer_standby)
            {
                //a esta altura debo tener bien medida la tension de alimentacion para poder determina dmax
                d = 0;
                main_state = MAIN_GENERATING;
                ChangeLed(LED_GENERATING);
                EXTIOn();
            }                
            break;

        case MAIN_GENERATING:
            if (!STOP_JUMPER)
            {
                if (seq_ready)
                {
                    seq_ready = 0;

                    if (undersampling < UNDERSAMPLING_TICKS)
                        undersampling++;
                    else
                    {
                        d = PID_roof (VOUT_200V, Vout_Sense, d, &ez1, &ez2);
                    
                        if (d < 0)
                            d = 0;

                        if (d > dmax)
                            d = dmax;

                        UpdateTIMSync (d);
                        dmax = UpdateDMAX(vin_filtered);    //TODO: luego meter el filtro en sync con mustras
                    }
                }    //cierra sequence
            }
            else
            {
                UpdateTIMSync (0);
                d = 0;
                timer_standby = 300;    //doy minimo 300ms para reactivar
                ChangeLed(LED_PROTECTED);
                main_state = MAIN_JUMPER_PROTECTED;
            }            
            break;

        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    //vuelvo a INIT
                    main_state = MAIN_SET_ZERO_CURRENT;
                    ChangeLed(LED_STANDBY);
                }
            }                
            break;

        case MAIN_OVERCURRENT:
            if (!timer_standby)
            {
                timer_standby = 100;
                if (LED)
                    LED_OFF;
                else
                    LED_ON;
            }

            if (STOP_JUMPER)
                main_state = MAIN_SET_ZERO_CURRENT;
            break;

        default:
            seq_ready = 0;            
            main_state = MAIN_SYNCHRO_ADC;
            break;
        }	//fin switch main_state

        if (!timer_standby)
        {
            timer_standby = 2000;
            sprintf (s_lcd, "VIN: %d, VOUT: %d, d: %d\r\n", Vin_Sense, Vout_Sense, d);
            Usart1Send(s_lcd);
        }

        if (current_excess)
        {
            current_excess = 0;
            d = 0;
            Usart1Send("\r\n Overcurrent!");
            main_state = MAIN_OVERCURRENT;
        }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
        }
            
            

        UpdateLed();
        
    }	//fin while 1
#endif //PRODUCTION_PRGRM

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

//hubo sobrecorriente, me llaman desde la interrupcion
void Overcurrent_Shutdown (void)
{
    //primero freno todos los PWM
    UpdateTIMSync(0);

    DISABLE_TIM3;
    DISABLE_TIM1;

    LED_ON;    //aviso con el led una vez que freno los pwm   

    //ahora aviso del error
    if (EXTI->PR & 0x00000010)    //Linea 4 es PROT_MOS_A
        current_excess = 4;
    else if (EXTI->PR & 0x00000020)    //Linea 5 es PROT_MOS_B
        current_excess = 5;
    else
        current_excess = 1;
}


#ifdef VER_2_0
//asi como esta tarda 800ns para ejecutar primera linea y 5.6us para la ultima linea
//TODO: cambiar todo lo que se pueda por MACROS
void EXTI4_15_IRQHandler(void)
{
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
#endif

#ifdef VER_1_2
void EXTI4_15_IRQHandler(void)
{
    if (EXTI->PR & 0x00000010)	//Line4
    {
        if (SENSE_MOSFET_A)
        {
            DisablePreload_MosfetA();
            UpdateTIM_MosfetA(0);
            EnablePreload_MosfetA();
            UpdateTIM_MosfetA(DUTY_FOR_DMAX);            
        }
        else if (SENSE_MOSFET_B)
        {
            DisablePreload_MosfetB();
            UpdateTIM_MosfetB(0);
            EnablePreload_MosfetB();
            UpdateTIM_MosfetB(DUTY_FOR_DMAX);
        }
        else
        {
            //llegue tarde o hay ruido
        }
        
        EXTI->PR |= 0x00000010;    //4
    }
    else if (EXTI->PR & 0x00000020)	//Line5
    {
        DisablePreload_MosfetA();
        DisablePreload_MosfetB();

        UpdateTIMSync(0);
        DISABLE_TIM3;
        DISABLE_TIM1;

        LED_ON;    //aviso con el led una vez que freno los pwm   

        //Overcurret Shutdown
        EXTI->PR |= 0x00000020;    //5
    }
}
#endif

#ifdef VER_1_1
void EXTI4_15_IRQHandler(void)
{
    Overcurrent_Shutdown();
    if ((EXTI->PR & 0x00000020) || (EXTI->PR & 0x00000010))	//Line4 or Line5
    {
        EXTI->PR |= 0x00000030;    //4 or 5
    }
}
#endif

#ifdef VER_1_0
void EXTI0_1_IRQHandler(void)
{
    if(EXTI->PR & 0x00000001)	//Line0
    {
        //reviso que mosfet generaba
        if (CTRL_M_A)
        {
            DisablePreload_MosfetA();
            UpdateTIM_MosfetA(0);
            EnablePreload_MosfetA();
            UpdateTIM_MosfetA(DUTY_FOR_DMAX);
        }
        else if (CTRL_M_B)
        {
            DisablePreload_MosfetB();
            UpdateTIM_MosfetB(0);
            EnablePreload_MosfetB();
            UpdateTIM_MosfetB(DUTY_FOR_DMAX);            
        }
        else
        {
            //llegue muy tarde con la INT
        }

        current_excess = 1;


        EXTI->PR |= 0x00000001;
    }
}
#endif


//------ EOF -------//
