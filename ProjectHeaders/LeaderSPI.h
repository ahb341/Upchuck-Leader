/****************************************************************************

  Header file for SPI test service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef LeaderSPI_H
#define LeaderSPI_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState, Waiting, Sending
}LeaderSPIState_t;

// Public Function Prototypes

bool InitLeaderSPI(uint8_t Priority);
bool PostLeaderSPI(ES_Event_t ThisEvent);
ES_Event_t RunLeaderSPI(ES_Event_t ThisEvent);
LeaderSPIState_t QueryLeaderSPI(void);

#endif /* LeaderSPI_H */

