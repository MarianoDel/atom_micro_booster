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


// Globals ---------------------------------------------------------------------
volatile unsigned char hard_overcurrent = 0;
int dma_sequence = 0;
int hard_stop_jumper = 0;
int hard_prot_mosfet = 0;
int hard_led_state = 0;


// Module Functions to Test ----------------------------------------------------
void TEST_Boost_Timeout (void);
void TEST_Boost_Loop (void);


// Module Auxiliary Functions for Tests ----------------------------------------
void EXTIOn (void);
void EXTIOff (void);
unsigned char DMASequenceReady (void);
void DMASequenceReadyReset (void);
unsigned char HARD_StopJumper (void);
unsigned char HARD_MosfetProtection (void);
void ChangeLed (unsigned char led);
void TIM_DisableMosfets (void);


// Module Functions ------------------------------------------------------------
int main (int argc, char *argv[])
{
    printf("Simple boost module tests\n");
    TEST_Boost_Timeout();
    TEST_Boost_Loop();    

    return 0;
}


void TEST_Boost_Loop (void)
{
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


void ChangeLed (unsigned char led)
{
    hard_led_state = led;
    printf("LED changed to: %d\n", led);
}


void TIM_DisableMosfets (void)
{
    printf("Mosfets disabled!\n");
}


//--- end of file ---//


