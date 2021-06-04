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
    boost_soft_overcurrent,
    boost_soft_overvoltage,
    boost_input_over_undervoltage
    
} boost_state_e;


#define BOOST_LED_INIT    1
#define BOOST_LED_SOFT_START    2
#define BOOST_LED_FULL_LOAD    3
#define BOOST_LED_JUMPER_PROT    4
#define BOOST_LED_HARD_OVERCURRENT    5
#define BOOST_LED_SOFT_OVERCURRENT    6
#define BOOST_LED_SOFT_OVERVOLTAGE    7
#define BOOST_LED_OVER_UNDERVOLTAGE    8


// Externals -------------------------------------------------------------------
extern volatile unsigned short timer_standby;
extern volatile unsigned char usart1_have_data;
extern volatile unsigned char hard_overcurrent;
extern volatile unsigned short adc_ch[];



// Globals ---------------------------------------------------------------------
volatile unsigned short boost_timeout = 0;
boost_state_e boost_state = boost_init;
ma8_u16_data_obj_t vin_sense_filter;
short duty = 0;
unsigned char soft_start_cntr = 0;
unsigned short seq_cntr = 0;


pid_data_obj_t voltage_pid;


// Module Private Functions ----------------------------------------------------
unsigned short BoostMaxDutyVinput (unsigned short vin);
unsigned short BoostMaxDutyLout (unsigned short vin, unsigned short vout);


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

        // check for soft overvoltage    TODO: not start from init next time
        if ((Vout_Sense > VOUT_SENSE_MAX_THRESHOLD) &&
            (boost_state != boost_soft_overvoltage))
        {
            TIM_DisableMosfets();
            boost_state = boost_soft_overvoltage;
            boost_timeout = 10000;
            ChangeLed(BOOST_LED_SOFT_OVERVOLTAGE);
        }
        
        // check for dmax allowed
        unsigned short vin_filtered = MA8_U16Circular(&vin_sense_filter, Vin_Sense);
        unsigned short dmax_vin = BoostMaxDutyVinput (vin_filtered);
        unsigned short dmax_lout = BoostMaxDutyLout (vin_filtered, Vout_Sense);
        // unsigned short dmax_lout = DUTY_FOR_DMAX;
        unsigned short dmax = 0;

        if (dmax_vin <= dmax_lout)
            dmax = dmax_vin;
        else
            dmax = dmax_lout;

        
        // check for overvoltage or undervoltage
        // if (boost_state != boost_input_over_undervoltage)
        // {
        //     if (seq_cntr < 8)    // give the filter some time
        //         seq_cntr++;
        //     else if ((vin_filtered > VIN_SENSE_MAX_THRESHOLD) ||
        //              (vin_filtered < VIN_SENSE_MIN_THRESHOLD))

        //     {
        //         TIM_DisableMosfets();
        //         boost_state = boost_input_over_undervoltage;
        //         boost_timeout = 10000;
        //         ChangeLed(BOOST_LED_OVER_UNDERVOLTAGE);
        //     }
        // }
        
        
        switch (boost_state)
        {
        case boost_init:
            ChangeLed(BOOST_LED_INIT);
            MA8_U16Circular_Reset(&vin_sense_filter);
            duty = 0;
            soft_start_cntr = 0;
            PID_Flush_Errors(&voltage_pid);
            // PID_Small_Ki_Flush_Errors(&voltage_pid);            

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
            if (Vout_Sense < VOUT_SENSE_SOFT_START_THRESHOLD)
            {
                if (soft_start_cntr)
                    soft_start_cntr--;
                else
                {
                    soft_start_cntr = 7;    //eight cycles before the change 8/24000 = 333us
                    if (duty < dmax)
                    {
                        duty++;
                        TIM_UpdateMosfetsSync(duty);
                    }
                }
            }
            // else    //disconect for 60 secs after soft start
            // {
            //     TIM_UpdateMosfetsSync(DUTY_NONE);
            //     boost_state = boost_jumper_protected;
            //     boost_timeout = 60000;
            // }
            else
            {
                voltage_pid.kp = 50;
                voltage_pid.ki = 1;
                voltage_pid.kd = 100;
                voltage_pid.last_d = duty;
                
                boost_state++;
                ChangeLed(BOOST_LED_FULL_LOAD);
            }
            break;

        case boost_full_load:
            voltage_pid.setpoint = VOUT_SENSE_SETPOINT;
            voltage_pid.sample = Vout_Sense;
            // duty = PI(&voltage_pid);
            duty = PID(&voltage_pid);            
            // duty = PID_Small_Ki(&voltage_pid);

            if (duty < 0)
                duty = 0;

            if (duty > dmax)
            {
                duty = dmax;
                voltage_pid.last_d = duty;
            }

            TIM_UpdateMosfetsSync(duty);
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

        case boost_soft_overvoltage:    //TODO: change this go to full load
            if (!boost_timeout)
            {
                boost_state = boost_init;
            }
            break;

        case boost_input_over_undervoltage:
            if (!boost_timeout)
            {
                boost_state = boost_init;
            }
            break;
            
        default:
            boost_state = boost_init;
            break;
            
        }    // end of switch
        
    }    // end of dma sequence ready

    //continuous checks
    if ((HARD_StopJumper()) && (boost_state != boost_jumper_protected))
    {
        TIM_DisableMosfets();
        boost_state = boost_jumper_protected;
        boost_timeout = 10000;
        ChangeLed(BOOST_LED_JUMPER_PROT);
    }

    if (hard_overcurrent)
    {
        TIM_DisableMosfets();
        boost_state = boost_hard_overcurrent;
        boost_timeout = 10000;
        hard_overcurrent = 0;
        EXTIOff();
        ChangeLed(BOOST_LED_HARD_OVERCURRENT);        
    }
}


// unsigned char tims_on = 0;
// void BoostLoopTestMosfet (void)
// {
//     if (DMASequenceReady())
//     {
//         DMASequenceReadyReset();
//         seq_cntr += 1;

//         if (seq_cntr > 48000)
//         {
//             seq_cntr = 0;
//             if (tims_on)
//             {
//                 LED_OFF;
//                 TIM_UpdateMosfetsSync(0);
//                 tims_on = 0;
//             }
//             else
//             {
//                 LED_ON;
//                 TIM_UpdateMosfetsSync(DUTY_10_PERCENT);
//                 tims_on = 1;
//             }
//         }
//     }
// }

void BoostFiltersInit (void)
{
    MA8_U16Circular_Reset(&vin_sense_filter);
}


void BoostTimeouts (void)
{
    if (boost_timeout)
        boost_timeout--;
    
}


// tmax = ae * bmax * np / vin
// ae = 153e-6
// bmax = 0.25
// np = 3
//
// max_duty = 1000
// duty = tmax * freq * max_duty
//
// adc_vref = 3.3
// adc_fullscale = 4095
// volts divider = 0.0909
// vin_adc = vin * volts_divider * adc_fullscale / adc_vref
// vin = vin_adc * adc_vref / (volts_divider * adc_fullscale)
//
// numerator modified (ae * bmax * np) * (volts_divider * adc_fullscale) = 0.042714
// denominator modified vin_adc * adc_ref
//
// num scaled = (0.042714/3.3) * freq * max_duty = 621294.88
#define K_TRAFO 621295
unsigned short BoostMaxDutyVinput (unsigned short vin)
{
    if (vin == 0)
        return 0;
    
    return K_TRAFO / vin;
}


// vin volts divider = 2255 @ 20V = 0.00887
// vin reflected divider = 22 * 0.00887 = 0.195
// vout volts divider = 3000 pts @ 350V = 0.1166
//
// tmax = Imax . Lm / delta_v
// Imax = 1.345A
// Lm = 1.8mHy
// max_duty = 1000
// duty = tmax * freq * max_duty
// num scaled = Imax * Lm * freq * max_duty = 116208
unsigned short BoostMaxDutyLout (unsigned short vin, unsigned short vout)
{
    unsigned int vin_reflected = vin * 195;
    vin_reflected = vin_reflected / 1000;

    unsigned int vout_scaled = vout * 117;
    vout_scaled = vout_scaled / 1000;

    if (vin_reflected <= vout_scaled)    //equals protect div by 0!
        return DUTY_FOR_DMAX;

    unsigned short delta_v = vin_reflected - vout_scaled;
    unsigned int duty = 116208 / delta_v;

    if (duty > DUTY_FOR_DMAX)
        return DUTY_FOR_DMAX;
    else
        return (unsigned short) duty;
    
}


//--- end of file ---//
