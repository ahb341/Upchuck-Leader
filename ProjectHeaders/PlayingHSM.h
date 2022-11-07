/****************************************************************************
 Header file for Hierarchical Sate Machine AKA StateCharts
 ****************************************************************************/

#ifndef PlayingHSM_H
#define PlayingHSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { MOVING_FWD, SHOOTING, MOVING_REV, RELOADING, ALIGNING_SHOT, ALIGNING_RELOAD } PlayingState_t ;



// Public Function Prototypes

ES_Event_t RunPlayingSM( ES_Event_t CurrentEvent );
void StartPlayingSM ( ES_Event_t CurrentEvent );
PlayingState_t QueryPlayingSM ( void );

#endif /* PlayingHSM_H */

