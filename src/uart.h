//---------------------------------------------
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    STM32F030
// ##
// #### UART.H ################################
//---------------------------------------------
#ifndef _UART_H_
#define _UART_H_

// Module Exported Types Constants and Macros ----------------------------------


// Module Exported Functions ---------------------------------------------------
unsigned char Usart1ReadBuffer (unsigned char *, unsigned short);

void Usart1Config(void);
void USART1_IRQHandler(void);
void Usart1Send (char *);
void Usart1SendUnsigned(unsigned char *, unsigned char);
void Usart1SendSingle(unsigned char );


#endif    /* _UART_H_ */

//--- end of file ---//

