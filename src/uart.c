//---------------------------------------------
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    STM32F030
// ##
// #### UART.C ################################
//---------------------------------------------

// Includes --------------------------------------------------------------------
#include "hard.h"
#include "uart.h"

#include <string.h>


// Module Private Types Constants and Macros -----------------------------------
#define USART1_CLK    (RCC->APB2ENR & 0x00004000)
#define USART1_CLK_ON    (RCC->APB2ENR |= 0x00004000)
#define USART1_CLK_OFF    (RCC->APB2ENR &= ~0x00004000)

#define USART_9600		5000
#define USART_115200		416
#define USART_250000		192


// Module Configs --------------------------------------------------------------


// Externals -------------------------------------------------------------------
extern volatile unsigned char usart1_have_data;


// Globals ---------------------------------------------------------------------
#define SIZEOF_TXDATA    128
#define SIZEOF_RXDATA    128
volatile unsigned char tx1buff [SIZEOF_TXDATA];
volatile unsigned char rx1buff [SIZEOF_RXDATA];

volatile unsigned char * ptx1;
volatile unsigned char * ptx1_pckt_index;
volatile unsigned char * prx1;


// Module Private Types & Macros -----------------------------------------------


// Module Private Functions ----------------------------------------------------


// Module Functions ------------------------------------------------------------
unsigned char Usart1ReadBuffer (unsigned char * bout, unsigned short max_len)
{
    unsigned int len;

    len = prx1 - rx1buff;

    if (len < max_len)
        len += 1;    //space for '\0' from int
    else
        len = max_len;

    memcpy(bout, (unsigned char *) rx1buff, len);

    //pointer adjust after copy
    prx1 = rx1buff;
    return (unsigned char) len;
}


void USART1_IRQHandler(void)
{
    unsigned char dummy;

    // USART Receiver mode -------------------------------------------
    if (USART1->ISR & USART_ISR_RXNE)
    {
        dummy = USART1->RDR & 0x0FF;

        if (prx1 < &rx1buff[SIZEOF_RXDATA - 1])
        {
            if ((dummy == '\n') || (dummy == '\r') || (dummy == 26))		//26 es CTRL-Z
            {
                *prx1 = '\0';
                usart1_have_data = 1;
            }
            else
            {
                *prx1 = dummy;
                prx1++;
            }
        }
        else
            prx1 = rx1buff;    //soluciona problema bloqueo con garbage
        
    }

    // USART Transmitter Mode ----------------------------------------
    if (USART1->CR1 & USART_CR1_TXEIE)
    {
        if (USART1->ISR & USART_ISR_TXE)
        {
            if ((ptx1 < &tx1buff[SIZEOF_TXDATA]) && (ptx1 < ptx1_pckt_index))
            {
                USART1->TDR = *ptx1;
                ptx1++;
            }
            else
            {
                ptx1 = tx1buff;
                ptx1_pckt_index = tx1buff;
                USART1->CR1 &= ~USART_CR1_TXEIE;
            }
        }
    }

    if ((USART1->ISR & USART_ISR_ORE) || (USART1->ISR & USART_ISR_NE) || (USART1->ISR & USART_ISR_FE))
    {
        USART1->ICR |= 0x0e;
        dummy = USART1->RDR;
    }
}


void Usart1Send (char * send)
{
    unsigned char i;

    i = strlen(send);
    Usart1SendUnsigned((unsigned char *) send, i);
}


void Usart1SendUnsigned(unsigned char * send, unsigned char size)
{
    if ((ptx1_pckt_index + size) < &tx1buff[SIZEOF_TXDATA])
    {
        memcpy((unsigned char *)ptx1_pckt_index, send, size);
        ptx1_pckt_index += size;
        USART1->CR1 |= USART_CR1_TXEIE;
    }
}


void Usart1SendSingle(unsigned char tosend)
{
    Usart1SendUnsigned(&tosend, 1);
}


void Usart1Config(void)
{
    if (!USART1_CLK)
        USART1_CLK_ON;

    ptx1 = tx1buff;
    ptx1_pckt_index = tx1buff;
    prx1 = rx1buff;

    USART1->BRR = USART_9600;
//	USART1->CR2 |= USART_CR2_STOP_1;	//2 bits stop
//	USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;
//	USART1->CR1 = USART_CR1_RXNEIE | USART_CR1_RE | USART_CR1_UE;	//SIN TX
    USART1->CR1 = USART_CR1_RXNEIE | USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;	//para pruebas TX

    unsigned int temp;
    temp = GPIOA->AFR[1];
    temp &= 0xFFFFF00F;
    temp |= 0x00000110;	//PA10 -> AF1 PA9 -> AF1
    GPIOA->AFR[1] = temp;

    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 7);
}


//--- end of file ---//
