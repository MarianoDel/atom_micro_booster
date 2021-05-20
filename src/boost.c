//------------------------------------------------
// #### PROJECT: O3 TREATMENT - Custom Board #####
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ##
// #### BOOST.C ##################################
//------------------------------------------------

// Includes --------------------------------------------------------------------
#include "boost.h"
#include "tim.h"
#include "dsp.h"
// #include "uart.h"

#include "adc.h"
#include "dma.h"

#include <stdio.h>


// Module Private Types Constants & Macros -------------------------------------
typedef enum {
    boost_init,
    boost_check_prot,
    boost_soft_start,
    boost_full_load,
    boost_jumper_protected,
    boost_hard_overcurrent,
    boost_soft_overcurrent    
    
} boost_state_e;


#define BOOST_LED_INIT    1
#define BOOST_LED_SOFT_START    2
#define BOOST_LED_FULL_LOAD    3
#define BOOST_LED_JUMPER_PROT    4
#define BOOST_LED_HARD_OVERCURRENT    5
#define BOOST_LED_SOFT_OVERCURRENT    6

#define I_SENSE_MAX_THRESHOLD    2000

// Externals -------------------------------------------------------------------
extern volatile unsigned short timer_standby;
extern volatile unsigned char usart1_have_data;
extern volatile unsigned char hard_overcurrent;
extern volatile unsigned short adc_ch[];



// Globals ---------------------------------------------------------------------
volatile unsigned short boost_timeout = 0;
boost_state_e boost_state = boost_init;
ma8_u16_data_obj_t vin_sense_filter;


// Module Private Functions ----------------------------------------------------


// Module Functions ------------------------------------------------------------
void BoostLoop (void)
{
    if (DMASequenceReady())
    {
        DMASequenceReadyReset();

        // check for soft overcurrent
        if ((I_Sense > I_SENSE_MAX_THRESHOLD) &&
            (boost_state != boost_soft_overcurrent))
        {
            TIM_DisableMosfets();
            boost_state = boost_soft_overcurrent;
            boost_timeout = 10000;
            ChangeLed(BOOST_LED_SOFT_OVERCURRENT);
        }


        unsigned short vin_filtered = MA8_U16Circular(&vin_sense_filter, Vin_Sense);
        // unsigned short dmax_vin = 

        
        
        switch (boost_state)
        {
        case boost_init:
            ChangeLed(BOOST_LED_INIT);
            MA8_U16Circular_Reset(&vin_sense_filter);

            boost_state++;
            break;

        case boost_check_prot:
            if (!HARD_MosfetProtection())
            {
                EXTIOn();
                boost_state++;
                ChangeLed(BOOST_LED_SOFT_START);
            }
            
            break;
            
        case boost_soft_start:            
            break;

        case boost_full_load:
            break;

        case boost_jumper_protected:
            if ((!boost_timeout) &&
                (!HARD_StopJumper()))
            {
                boost_state = boost_init;
            }
            break;

        case boost_hard_overcurrent:
            if ((!boost_timeout) &&
                (!HARD_MosfetProtection()))
            {
                boost_state = boost_init;
            }
            break;

        case boost_soft_overcurrent:
            if (!boost_timeout)
            {
                boost_state = boost_init;
            }
            break;
            
        default:
            boost_state = boost_init;
            break;
        }
    }

    //continuous checks
    if ((HARD_StopJumper()) && (boost_state != boost_jumper_protected))
    {
        TIM_DisableMosfets();
        boost_state = boost_jumper_protected;
        boost_timeout = 10000;
        
        // DisablePreload_MosfetA;
        // DisablePreload_MosfetB;

        // UpdateTIMSync (DUTY_NONE);
        // boost_state = boost_jumper_protected;
        // boost_timeout = 10000;

        // EnablePreload_MosfetA;
        // EnablePreload_MosfetB;

        ChangeLed(BOOST_LED_JUMPER_PROT);
    }

    if (hard_overcurrent)
    {
        TIM_DisableMosfets();
        boost_state = boost_hard_overcurrent;
        boost_timeout = 10000;
        // DisablePreload_MosfetA;
        // DisablePreload_MosfetB;

        // UpdateTIMSync (DUTY_NONE);
        // boost_state = boost_hard_overcurrent;
        // boost_timeout = 10000;

        // EnablePreload_MosfetA;
        // EnablePreload_MosfetB;

        hard_overcurrent = 0;
        EXTIOff();
        ChangeLed(BOOST_LED_HARD_OVERCURRENT);        
        
    }
}


void BoostFiltersInit (void)
{
    MA8_U16Circular_Reset(&vin_sense_filter);
}


void BoostTimeouts (void)
{
    if (boost_timeout)
        boost_timeout--;
    
}


//--- end of file ---//
