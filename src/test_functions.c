//------------------------------------------------
// #### PROJECT: O3 TREATMENT - Custom Board #####
// ## Internal Test Functions Module
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ##
// #### TEST_FUNCTIONS.C #########################
//------------------------------------------------

// Includes --------------------------------------------------------------------
#include "test_functions.h"
#include "hard.h"
#include "tim.h"
#include "uart.h"
#include "gpio.h"

#include "adc.h"
#include "dma.h"

#include <stdio.h>


// Externals -------------------------------------------------------------------
extern volatile unsigned short timer_standby;
extern volatile unsigned char usart1_have_data;
extern volatile unsigned char hard_overcurrent;
extern volatile unsigned short adc_ch[];



// Globals ---------------------------------------------------------------------


// Module Private Types & Macros -----------------------------------------------


// Module Private Functions ----------------------------------------------------


// Module Functions ------------------------------------------------------------
void TF_Led (void)
{
    while (1)
    {
        if (LED)
            LED_OFF;
        else
            LED_ON;

        Wait_ms(1000);
    }
}


void TF_Led_Jumper (void)
{
    while (1)
    {
        if (STOP_JUMPER)
            LED_ON;
        else
            LED_OFF;

        Wait_ms(100);
    }
}


void TF_Led_Blinking (void)
{
    ChangeLed(2);
        
    while(1)
        UpdateLed();
}


void TF_Usart1_Tx (void)
{
    Usart1Config();

    while (1)
    {
        LED_ON;
        Usart1Send("Usart1 test...\n");
        Wait_ms(1000);
        WelcomeCodeFeatures();
        LED_OFF;

        Wait_ms(5000);
    }
}


void TF_Tim_Channels (void)
{
    TIM_1_Init ();    // mosfet Ctrol_M_B
    TIM_3_Init ();    // mosfet Ctrol_M_A & synchro ADC

    EnablePreload_MosfetA;
    EnablePreload_MosfetB;

    UpdateTIMSync (DUTY_10_PERCENT);
    
    while (1);
}


void TF_Prot_Mosfet (void)
{
    while (1)
    {
        if (PROT_MOS)
            LED_ON;
        else
            LED_OFF;

        Wait_ms(100);
    }
}


void TF_Prot_Mosfet_Int (void)
{
    EXTIOn();
    
    while (1)
    {
        if (hard_overcurrent)
        {
            if (LED)
                LED_OFF;
            else
                LED_ON;

            hard_overcurrent = 0;
        }
    }
}


void TF_Usart1_Adc_Dma (void)
{
    //-- TIM1 Init synchro ADC
    TIM_1_Init();

    //-- Usart Init
    Usart1Config();

    //-- ADC Init
    AdcConfig();

    //-- DMA configuration and Init
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;

    unsigned short cntr = 0;
    char s_to_send [100] = { 0 };
    Usart1Send("\nTesting ADC with dma transfers...\n");

    while (1)
    {
        if (sequence_ready)
        {
            sequence_ready_reset;
            if (cntr < 10000)
                cntr++;
            else
            {
                sprintf(s_to_send, "Vin_Sense: %d Vout_Sense: %d I_Sense: %d\n",
                        Vin_Sense,
                        Vout_Sense,
                        I_Sense);
                
                Usart1Send(s_to_send);
                cntr = 0;
            }

            // for sampling time determination
            if (LED)
                LED_OFF;
            else
                LED_ON;
        }            
    }
}


//--- end of file ---//
