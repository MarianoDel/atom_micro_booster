//------------------------------------------------
// #### MICROINVERTER PROJECT - Custom Board #####
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ##
// #### BOOST.H ##################################
//------------------------------------------------

// Prevent recursive inclusion -------------------------------------------------
#ifndef _BOOST_H_
#define _BOOST_H_


// Exported Types Constants and Macros -----------------------------------------
#define OUTPUT_SETPOINT    3000
#define VOUT_SENSE_SOFT_START_THRESHOLD    2800

#define I_SENSE_MAX_THRESHOLD    2000
#define VOUT_SENSE_MAX_THRESHOLD    3500


// Exported Functions ----------------------------------------------------------
void BoostLoop (void);
void BoostTimeouts (void);
void BoostFiltersInit (void);


#endif    /* _BOOST_H_ */

//--- end of file ---//

