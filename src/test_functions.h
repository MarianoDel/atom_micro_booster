//------------------------------------------------
// #### MICROINVERTER PROJECT - Custom Board ####
// ## Internal Test Functions Module
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ##
// #### TEST_FUNCTIONS.H #########################
//------------------------------------------------

// Prevent recursive inclusion -------------------------------------------------
#ifndef _TEST_FUNTIONS_H_
#define _TEST_FUNTIONS_H_


// Exported Types Constants and Macros -----------------------------------------



// Exported Functions ----------------------------------------------------------
void TF_Led (void);
void TF_Led_Jumper (void);
void TF_Led_Blinking (void);

void TF_Usart1_Tx (void);
void TF_Usart1_Tx_Single (void);
void TF_Usart1_Adc_Dma (void);

void TF_Tim_Channels (void);
void TF_Prot_Mosfet (void);
void TF_Prot_Mosfet_Int (void);


#endif    /* _TEST_FUNTIONS_H_ */

//--- end of file ---//

