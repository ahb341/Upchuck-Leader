/****************************************************************************
 Header file for Hierarchical Sate Machine AKA StateCharts
 ****************************************************************************/

#ifndef IdentifyingHSM_H
#define IdentifyingHSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum {ALIGN, IDLING} IdentifyingState_t ;



// Public Function Prototypes

ES_Event_t RunIdentifyingSM( ES_Event_t CurrentEvent );
void StartIdentifyingSM ( ES_Event_t CurrentEvent );
IdentifyingState_t QueryIdentifyingSM ( void );

#endif /* IdentifyingHSM_H */

