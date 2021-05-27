//---------------------------------------------
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    TEST PLATFORM FOR FIRMWARE
// ##
// #### TESTS_BOOST.C #########################
//---------------------------------------------

// Includes Modules for tests --------------------------------------------------
#include "boost.h"
// #include "pwm_defs.h"
// #include "dsp.h"
// #include "tests_vector_utils.h"

// Auxiliary tests modules -----------------------------------------------------
#include "tests_ok.h"

#include <stdio.h>
#include <math.h>


// Types Constants and Macros --------------------------------------------------
#define BOOST_LED_INIT    1
#define BOOST_LED_SOFT_START    2
#define BOOST_LED_FULL_LOAD    3
#define BOOST_LED_JUMPER_PROT    4
#define BOOST_LED_HARD_OVERCURRENT    5
#define BOOST_LED_SOFT_OVERCURRENT    6


// Externals -------------------------------------------------------------------
extern volatile unsigned short boost_timeout;
extern int boost_state;
extern unsigned short BoostMaxDutyVinput (unsigned short vin);


// Globals ---------------------------------------------------------------------
volatile unsigned char hard_overcurrent = 0;
int dma_sequence = 0;
int hard_stop_jumper = 0;
int hard_prot_mosfet = 0;
int hard_led_state = 0;
unsigned short adc_ch[3] = { 0 };


// Module Functions to Test ----------------------------------------------------
void TEST_Boost_Timeout (void);
void TEST_Boost_Loop (void);
void TEST_MaxTimeVinput (void);
void TEST_MaxTimeLout (void);


// Module Auxiliary Functions for Tests ----------------------------------------
float MaxTimeVinput (float vin_volts);
float MaxTimeLout (float vin_volts, float vout_volts);
int MaxDuty (float time_secs, int freq);


void EXTIOn (void);
void EXTIOff (void);
unsigned char DMASequenceReady (void);
void DMASequenceReadyReset (void);
unsigned char HARD_StopJumper (void);
unsigned char HARD_MosfetProtection (void);
void ChangeLed (unsigned char led);
void TIM_DisableMosfets (void);
void TIM_UpdateMosfetsSync (unsigned short new_pwm);


// Module Functions ------------------------------------------------------------
int main (int argc, char *argv[])
{
    printf("Simple boost module tests\n");
    TEST_Boost_Timeout();
    TEST_Boost_Loop();
    TEST_MaxTimeVinput ();
    TEST_MaxTimeLout ();    

    return 0;
}


void TEST_Boost_Loop (void)
{
    BoostFiltersInit ();
    
    printf("Testing boost loop\n");
    dma_sequence = 1;
    BoostLoop();
    
    printf("boost loop init: ");    
    if ((boost_state == 1) && (!dma_sequence))
        PrintOK();
    else
    {
        PrintERR();
        printf("boost_state expected 1, getted %d\n", boost_state);
    }

    dma_sequence = 1;
    hard_prot_mosfet = 1;
    BoostLoop();
    printf("boost loop protection: ");        
    if ((boost_state == 1) &&
        (!dma_sequence) &&
        (hard_led_state == BOOST_LED_INIT))
        PrintOK();
    else
    {
        PrintERR();
        printf("boost_state expected 1, getted %d\n", boost_state);
    }

    dma_sequence = 1;
    hard_prot_mosfet = 0;
    BoostLoop();
    printf("boost loop no protection: ");        
    if ((boost_state == 2) &&
        (!dma_sequence) &&
        (hard_led_state == BOOST_LED_SOFT_START))
        PrintOK();
    else
    {
        PrintERR();
        printf("boost_state expected 2, getted %d\n", boost_state);
    }


    dma_sequence = 1;
    hard_prot_mosfet = 0;
    hard_overcurrent = 1;
    BoostLoop();
    printf("boost loop hard overcurrent: ");
    if ((boost_state == 5) &&
        (!dma_sequence) &&
        (hard_led_state == BOOST_LED_HARD_OVERCURRENT))
        PrintOK();
    else
    {
        PrintERR();
        printf("boost_state expected 5, getted %d\n", boost_state);
    }


    dma_sequence = 1;
    hard_prot_mosfet = 0;
    hard_overcurrent = 0;
    boost_timeout = 0;
    BoostLoop();
    printf("boost loop hard overcurrent ended: ");    
    if ((boost_state == 0) && (!dma_sequence))
        PrintOK();
    else
    {
        PrintERR();
        printf("boost_state expected 0, getted %d\n", boost_state);
    }

    dma_sequence = 1;
    hard_prot_mosfet = 0;
    hard_stop_jumper = 1;
    BoostLoop();
    printf("boost loop jumper protected: ");
    if ((boost_state == 4) &&
        (!dma_sequence) &&
        (hard_led_state == BOOST_LED_JUMPER_PROT))
        PrintOK();
    else
    {
        PrintERR();
        printf("boost_state expected 4, getted %d\n", boost_state);
    }
    
    dma_sequence = 1;
    hard_prot_mosfet = 0;
    hard_stop_jumper = 0;
    boost_timeout = 0;
    BoostLoop();
    printf("boost loop jumper protected ended: ");    
    if ((boost_state == 0) && (!dma_sequence))
        PrintOK();
    else
    {
        PrintERR();
        printf("boost_state expected 0, getted %d\n", boost_state);
    }
    
}


void TEST_Boost_Timeout (void)
{
    printf("Testing boost timeout not finish: ");
    boost_timeout = 100;

    for (int i = 0; i < 99; i++)
        BoostTimeouts();

    if (boost_timeout == 1)
        PrintOK();
    else
        PrintERR();

    printf("Testing boost timeout finished: ");

    for (int i = 0; i < 99; i++)
        BoostTimeouts();

    if (!boost_timeout)
        PrintOK();
    else
        PrintERR();
    
}


void TEST_MaxTimeVinput (void)
{
    float vin_input = 20.0;
    float time_s = MaxTimeVinput(vin_input);
    printf("max time allowed for vin_input: %.02fV time: %fus\n",
           vin_input,
           time_s * 1e6);

    int freq_selected = 48000;
    int duty_max = MaxDuty (time_s, freq_selected); 
    printf("max duty for freq: %dHz duty: %d\n",
           freq_selected,
           duty_max);

    float vin_scaled = vin_input * 0.0909;
    float adc = vin_scaled * 4095 / 3.3;
    unsigned short vin_points = (unsigned short) adc;
    printf("embedded dmax for vin: %.2fV vin_scaled: %.2fV vin_points: %d\n",
           vin_input,
           vin_scaled,
           vin_points);

    unsigned short dmax_points = BoostMaxDutyVinput (vin_points);
    printf("embedded dmax result: %d\n", dmax_points);

    int errors = 0;
    for (int i = 20; i < 41; i++)
    {
        // local calcs
        vin_input = i;
        time_s = MaxTimeVinput(vin_input);
        duty_max = MaxDuty (time_s, freq_selected);

        // embedded calcs
        vin_scaled = vin_input * 0.0909;
        adc = vin_scaled * 4095 / 3.3;
        vin_points = (unsigned short) adc;
        dmax_points = BoostMaxDutyVinput (vin_points);

        if (duty_max != dmax_points)
        {
            i = 41;
            errors = 1;
        }
    }

    printf("Test loop embedded dmax for vinput: ");
    if (!errors)
        PrintOK();
    else
    {
        PrintERR();
        printf("vin_input: %.2f duty_max: %d embedded getted: %d\n",
               vin_input,
               duty_max,
               dmax_points);
    }

}


void TEST_MaxTimeLout (void)
{
    float vin_input = 20.0;
    float vout_input = 350.0;
    float time_s = MaxTimeLout(vin_input, vout_input);
    printf("max time allowed for vin: %.02fV vout: %.02fV time: %fus\n",
           vin_input,
           vout_input,
           time_s * 1e6);

    int freq_selected = 48000;
    int duty_max = MaxDuty (time_s, freq_selected); 
    printf("max duty for freq: %dHz duty: %d\n",
           freq_selected,
           duty_max);

    float vin_scaled = vin_input * 0.0909;
    float adc_vin = vin_scaled * 4095 / 3.3;
    unsigned short vin_points = (unsigned short) adc_vin;
    printf("embedded vin: %.2fV vin_scaled: %.2fV vin_points: %d\n",
           vin_input,
           vin_scaled,
           vin_points);

    float vout_scaled = vout_input * 0.0069;
    float adc_vout = vout_scaled * 4095 / 3.3;
    unsigned short vout_points = (unsigned short) adc_vout;
    printf("embedded vout: %.2fV vout_scaled: %.2fV vout_points: %d\n",
           vout_input,
           vout_scaled,
           vout_points);
    
    unsigned short dmax_points = BoostMaxDutyLout (vin_points, vout_points);
    printf("embedded dmax result: %d\n", dmax_points);

    int errors = 0;
    int distance = 0;
    for (int i = 0; i < 440; i+=10)
    {
        // local calcs
        vout_input = i;
        time_s = MaxTimeLout(vin_input, vout_input);
        duty_max = MaxDuty (time_s, freq_selected);

        // embedded calcs
        vout_scaled = vout_input * 0.0069;
        adc_vout = vout_scaled * 4095 / 3.3;
        vout_points = (unsigned short) adc_vout;
        dmax_points = BoostMaxDutyLout (vin_points, vout_points);

        // 5 point distance
        distance = duty_max - dmax_points;
        if ((distance > 2) || (distance < -2))
        {
            i = 440;
            errors = 1;
        }
    }

    printf("Test loop embedded dmax for Lout: ");
    if (!errors)
        PrintOK();
    else
    {
        PrintERR();
        printf("vin_input: %.2f vout_input: %.2f duty_max: %d embedded getted: %d\n",
               vin_input,
               vout_input,
               duty_max,
               dmax_points);
    }

}


float MaxTimeVinput (float vin_volts)
{
    float bmax = 0.25;
    float ae = 153e-6;
    int np = 3;

    float max_time = bmax * ae * np / vin_volts;

    return max_time;
}


float MaxTimeLout (float vin_volts, float vout_volts)
{
    float imax = 1.345;
    float lm = 1.8e-3;
    int nrel = 11 * 2;    //nsec = 33, npri = 3, double transformer

    float vin_reflected = vin_volts * nrel;

    float max_time = 0.0;
    if (vin_reflected < vout_volts)
        max_time = 1. / 48000;
    else
        max_time = imax * lm / (vin_reflected - vout_volts);

    return max_time;
}


int MaxDuty (float time_secs, int freq)
{
    int max_duty_allowed = 450;
    float period = freq;
    period = 1 / period;
    float duty = time_secs * 1000 / period;

    if (duty > max_duty_allowed)
        duty = max_duty_allowed;
    
    return (int) duty;
}


void EXTIOn (void)
{
    printf("EXTI is on!\n");
}


void EXTIOff (void)
{
    printf("EXTI is off!\n");
}


unsigned char DMASequenceReady (void)
{
    if (dma_sequence)
        return 1;
    else
        return 0;
}


void DMASequenceReadyReset (void)
{
    dma_sequence = 0;
}


unsigned char HARD_StopJumper (void)
{
    if (hard_stop_jumper)
        return 1;
    else
        return 0;
}


unsigned char HARD_MosfetProtection(void)
{
    if (hard_prot_mosfet)
        return 1;
    else
        return 0;
    
}


char led_strings [8][30] = { {"LED_NO_BLINKING"},
                             {"BOOST_LED_INIT"},
                             {"BOOST_LED_SOFT_START"},
                             {"BOOST_LED_FULL_LOAD"},
                             {"BOOST_LED_JUMPER_PROT"},
                             {"BOOST_LED_HARD_OVERCURRENT"},
                             {"BOOST_LED_SOFT_OVERCURRENT"},
                             {"BOOST_LED_SOFT_OVERVOLTAGE"} };

void ChangeLed (unsigned char led)
{
    hard_led_state = led;
    printf("LED changed to: %d %s\n", led, led_strings[led]);
}


void TIM_DisableMosfets (void)
{
    printf("Mosfets disabled!\n");
}


void TIM_UpdateMosfetsSync (unsigned short new_pwm)
{
    printf("New Mosfets duty: %d\n", new_pwm);
}

                             
//--- end of file ---//


