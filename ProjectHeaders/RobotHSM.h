/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef RobotHSM_H
#define RobotHSM_H

// Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

// State definitions for use with the query function
typedef enum { WAITING, IDENTIFYING, PLAYING, GAME_OVER } RobotState_t ;

// Public Function Prototypes
bool InitRobotSM ( uint8_t Priority );
bool PostRobotSM( ES_Event_t ThisEvent );
ES_Event_t RunRobotSM( ES_Event_t CurrentEvent );
void StartRobotSM ( ES_Event_t CurrentEvent );
RobotState_t  QueryRobotSM ( void );

// Event checkers
bool CheckStartButtonPressed(void);
bool CheckFireUpdate(void);
bool CheckMovementUpdate(void);


#endif /*RobotHSM_H */

