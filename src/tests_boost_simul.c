//-------------------------------------------------
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    TEST PLATFORM FOR FIRMWARE
// ##
// #### TESTS_BOOST_SIMUL.C #######################
//-------------------------------------------------

// Includes Modules for tests --------------------------------------------------
#include "boost.h"
// #include "pwm_defs.h"
// #include "dsp.h"

// Includes tests helper modules for the tests ---------------------------------
#include "tests_vector_utils.h"
#include "tests_recursive_utils.h"


#include <stdio.h>
#include <math.h>
#include <stdint.h>


// Types Constants and Macros --------------------------------------------------
// Select the sampling frequency used for Plant conversion (../booster_analog_digi.py)
#define SAMPLING_FREQ    24000

// Select simulation lenght in seconds
#define SIMULATION_LENGTH    0.1

// #define VECTOR_LENGTH    2400    //(SAMPLING_FREQ * SIMULATION_LENGTH)
#define VECTOR_LENGTH    ((int)(SAMPLING_FREQ * SIMULATION_LENGTH))

#define K_VOLTS_OUTPUT    0.0069    //volts sensed on output
#define K_VOLTS_INPUT    0.0909    //volts sensed on input

// Externals -------------------------------------------------------------------


// Globals ---------------------------------------------------------------------
// real vectors
float vinput[VECTOR_LENGTH] = { 0 };
float vinput_applied[VECTOR_LENGTH] = { 0 };
float voutput[VECTOR_LENGTH] = { 0 };
float reference_voltage [VECTOR_LENGTH] = { 0 };

// adc and others vectors
unsigned short vinput_adc[VECTOR_LENGTH] = { 0 };
unsigned short voutput_adc[VECTOR_LENGTH] = { 0 };
unsigned short reference [VECTOR_LENGTH] = { 0 };
unsigned short duty_applied [VECTOR_LENGTH] = { 0 };


volatile unsigned char hard_overcurrent = 0;
int dma_sequence = 0;
int hard_stop_jumper = 0;
int hard_prot_mosfet = 0;
int hard_led_state = 0;
unsigned short adc_ch[3] = { 0 };
unsigned short duty_doubled = 0;


// Tests Functions -------------------------------------------------------------
void TestBoostCloseLoop (void);
void Boost_Step_Response (void);
void Boost_Step_Response_Duty (void);

float Plant_Out_Recursive (float in);
void Plant_Out_Recursive_Reset (void);


// Module Auxiliary Functions for Tests ----------------------------------------
unsigned short Adc12BitsConvertion (float );

void EXTIOn (void);
void EXTIOff (void);

unsigned char DMASequenceReady (void);
void DMASequenceReadyReset (void);
void DMASequenceSet (void);

unsigned char HARD_StopJumper (void);
unsigned char HARD_MosfetProtection (void);
void ChangeLed (unsigned char led);
void TIM_DisableMosfets (void);
void TIM_UpdateMosfetsSync (unsigned short new_pwm);


// Module Functions ------------------------------------------------------------
int main (int argc, char *argv[])
{
    printf("Start of Analog simulations...\n");
    // Boost_Step_Response();
    // Boost_Step_Response_Duty();
    TestBoostCloseLoop();

    return 0;
}


void TestBoostCloseLoop (void)
{
    printf("\nBooster stage Close Loop Test\n");
    Plant_Out_Recursive_Reset();

    printf("init boost module\n");
    BoostFiltersInit ();

    printf("init voltage input from panels\n");
    for (int i = 0; i < VECTOR_LENGTH; i++)
        vinput[i] = 30.0;

    printf("init reference vector\n");
    for (int i = 0; i < VECTOR_LENGTH; i++)
    {
        reference[i] = VOUT_SENSE_SETPOINT;
        reference_voltage[i] = VOUT_SENSE_SETPOINT * 3.3 / (4095 * K_VOLTS_OUTPUT);
    }
    
    printf("execute boost loop\n");
    
    unsigned short last_output_adc = 0;
    for (int i = 0; i < VECTOR_LENGTH; i++)
    {
        // inputs to the system
        vinput_adc[i] = Adc12BitsConvertion(vinput[i] * K_VOLTS_INPUT);
        adc_ch[0] = vinput_adc[i];    //Vin_Sense
        adc_ch[1] = last_output_adc;    //Vout_Sense
        DMASequenceSet ();

        // the boost resolution
        BoostLoop();

        // get the boost module output
        duty_applied[i] = duty_doubled;

        // some calcs for the plant
        vinput_applied[i] = vinput[i] * 33 * duty_doubled / 1000;
        voutput[i] = Plant_Out_Recursive(vinput_applied[i]);

        voutput_adc[i] = Adc12BitsConvertion(voutput[i] * K_VOLTS_OUTPUT);
        last_output_adc = voutput_adc[i];
    }


    // real signals
    // ShowVectorFloat("\nVector vinput_applied:\n", vinput_applied, VECTOR_LENGTH);
    // ShowVectorFloat("\nVector plant output:\n", voutput, VECTOR_LENGTH);

    // adc signals
    // ShowVectorUShort("\nVector vinput ADC:\n", vinput_adc, VECTOR_LENGTH);    
    ShowVectorUShort("\nVector plant output ADC:\n", voutput_adc, VECTOR_LENGTH);
    // ShowVectorUShort("\nVector duty_applied:\n", duty_applied, VECTOR_LENGTH);    
    

    // int error [VECTOR_LENGTH];
    // for (int i = 0; i < VECTOR_LENGTH; i++)
    //     error[i] = reference[i] - voutput_adc[i];

    // ShowVectorInt("\nPlant output error:\n", error, VECTOR_LENGTH);

    ///////////////////////////
    // Backup Data to a file //
    ///////////////////////////
    FILE * file = fopen("data.txt", "w");

    if (file == NULL)
    {
        printf("data file not created!\n");
        return;
    }

    // real vectors
    Vector_Float_To_File(file, "vinput_applied", vinput_applied, VECTOR_LENGTH);
    Vector_Float_To_File(file, "voutput", voutput, VECTOR_LENGTH);
    Vector_Float_To_File(file, "ref", reference_voltage, VECTOR_LENGTH);        
    
    // adc signals vectors
    Vector_UShort_To_File(file, "reference", reference, VECTOR_LENGTH);
    Vector_UShort_To_File(file, "duty_applied", duty_applied, VECTOR_LENGTH);
    Vector_UShort_To_File(file, "vinput_adc", vinput_adc, VECTOR_LENGTH);
    Vector_UShort_To_File(file, "voutput_adc", voutput_adc, VECTOR_LENGTH);

    printf("\nRun by hand python3 simul_booster_close_loop.py\n");
    
}


void Boost_Step_Response_Duty (void)
{
    printf("\nBooster stage Step Response with duty = 0.45\n");
    Plant_Out_Recursive_Reset();
    
    for (int i = 0; i < VECTOR_LENGTH; i++)
    {
        //20.0V input
        //nrel = 22
        //duty = 0.45; duty_doubled = 0.9
        vinput_applied[i] = 396.;
    }

    for (int i = 0; i < VECTOR_LENGTH; i++)
    {
        voutput[i] = Plant_Out_Recursive(vinput_applied[i]);
        voutput_adc[i] = Adc12BitsConvertion(voutput[i] * K_VOLTS_OUTPUT);
    }
    

    // ShowVectorFloat("\nVector voltage input:\n", vinput_applied, VECTOR_LENGTH);
    // ShowVectorFloat("\nVector plant output:\n", voutput, VECTOR_LENGTH);
    ShowVectorUShort("\nVector plant output adc:\n", voutput_adc, VECTOR_LENGTH);

    ///////////////////////////
    // Backup Data to a file //
    ///////////////////////////
    FILE * file = fopen("data.txt", "w");

    if (file == NULL)
    {
        printf("data file not created!\n");
        return;
    }

    Vector_Float_To_File(file, "vinput_applied", vinput_applied, VECTOR_LENGTH);
    Vector_Float_To_File(file, "voutput", voutput, VECTOR_LENGTH);
    Vector_UShort_To_File(file, "adc_output", voutput_adc, VECTOR_LENGTH);    

    printf("\nRun by hand python3 simul_booster_duty.py\n");

}


void Boost_Step_Response (void)
{
    printf("\nBooster stage Step Response\n");
    Plant_Out_Recursive_Reset();
    
    for (int i = 0; i < VECTOR_LENGTH; i++)
    {
        vinput_applied[i] = 1.;
    }

    for (int i = 0; i < VECTOR_LENGTH; i++)
    {
        voutput[i] = Plant_Out_Recursive(vinput_applied[i]);
    }
    

    ShowVectorFloat("\nVector voltage input:\n", vinput_applied, VECTOR_LENGTH);
    ShowVectorFloat("\nVector plant output:\n", voutput, VECTOR_LENGTH);

    ///////////////////////////
    // Backup Data to a file //
    ///////////////////////////
    FILE * file = fopen("data.txt", "w");

    if (file == NULL)
    {
        printf("data file not created!\n");
        return;
    }

    Vector_Float_To_File(file, "vinput_applied", vinput_applied, VECTOR_LENGTH);
    Vector_Float_To_File(file, "voutput", voutput, VECTOR_LENGTH);

    printf("\nRun by hand python3 simul_booster.py\n");

}


// Filter stage for Boost boost_analog_digi.py Vinput = 1
//
// TransferFunctionDiscrete(
// array([0.01457066, 0.01456314]),
// array([ 1.        , -1.96932006,  0.99845386]),
// dt: 4.1666666666666665e-05
// )
#define B_SIZE    2
#define A_SIZE    3
float b_vector [B_SIZE] = { 0.01457066 , 0.01456314 };
float a_vector [A_SIZE] = { 1., -1.96932006, 0.99845386 };
float ins_vector [B_SIZE] = { 0.0 };
float outs_vector [A_SIZE] = { 0.0 };
recursive_filter_t boost_t;
void Plant_Out_Recursive_Reset (void)
{
    boost_t.b_params = b_vector;
    boost_t.a_params = a_vector;
    boost_t.b_size = B_SIZE;
    boost_t.a_size = A_SIZE;
    boost_t.last_inputs = ins_vector;
    boost_t.last_outputs = outs_vector;    
    Recursive_Filter_Float_Reset(&boost_t);
}


float Plant_Out_Recursive (float in)
{
    return Recursive_Filter_Float(&boost_t, in);
}


unsigned short Adc12BitsConvertion (float sample)
{
    if (sample > 0.0001)
    {
        sample = sample / 3.3;
        sample = sample * 4095;
        
        if (sample > 4095)
            sample = 4095;
    }
    else
        sample = 0.0;

    return (unsigned short) sample;
    
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


void DMASequenceSet (void)
{
    dma_sequence = 1;
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


char led_strings [9][30] = { {"LED_NO_BLINKING"},
                             {"BOOST_LED_INIT"},
                             {"BOOST_LED_SOFT_START"},
                             {"BOOST_LED_FULL_LOAD"},
                             {"BOOST_LED_JUMPER_PROT"},
                             {"BOOST_LED_HARD_OVERCURRENT"},
                             {"BOOST_LED_SOFT_OVERCURRENT"},
                             {"BOOST_LED_SOFT_OVERVOLTAGE"},
                             {"BOOST_LED_OVER_UNDERVOLTAGE"} };

void ChangeLed (unsigned char led)
{
    hard_led_state = led;
    printf("LED changed to: %d %s\n", led, led_strings[led]);
}


void TIM_DisableMosfets (void)
{
    duty_doubled = 0;
}


void TIM_UpdateMosfetsSync (unsigned short new_pwm)
{
    duty_doubled = new_pwm + new_pwm;
}


//--- end of file ---//


