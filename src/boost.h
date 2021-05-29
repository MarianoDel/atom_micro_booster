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
//meas 3883 pts 34.3V
#define VIN_SENSE_MAX_THRESHOLD    4000    //4000 --> 35.3V
#define VIN_SENSE_MIN_THRESHOLD    2264    //2264 --> 20V

// #define VOUT_SENSE_SOFT_START_THRESHOLD    2800
// #define VOUT_SENSE_SETPOINT    3000
// #define VOUT_SENSE_MAX_THRESHOLD    3500

// meas on 26-05-2021
// 1190 pts 200V
// 1100 pts 188V feedback set
// 1250 pts 211V feedback set
// #define VOUT_SENSE_SOFT_START_THRESHOLD    1100
// #define VOUT_SENSE_SETPOINT    1250
// #define VOUT_SENSE_MAX_THRESHOLD    1500

// meas on 28-05-2021
// 1850 pts 255V feedback
// #define VOUT_SENSE_SOFT_START_THRESHOLD    1628    //(setpoint * 0.88)
// #define VOUT_SENSE_SETPOINT    1850
// #define VOUT_SENSE_MAX_THRESHOLD    2200        //(setpoint * 1.2)

#define VOUT_SENSE_SOFT_START_THRESHOLD    2442    //(setpoint * 0.88)
#define VOUT_SENSE_SETPOINT    2775
#define VOUT_SENSE_MAX_THRESHOLD    3330        //(setpoint * 1.2)

#define I_SENSE_MAX_THRESHOLD    2000



// Exported Functions ----------------------------------------------------------
void BoostLoop (void);
void BoostTimeouts (void);
void BoostFiltersInit (void);
void BoostLoopTestMosfet (void);


#endif    /* _BOOST_H_ */

//--- end of file ---//

