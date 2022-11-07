/****************************************************************************

  Header file for Test Harness Service0
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef BeaconTestHarness_H
#define BeaconTestHarness_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Port.h"                // needed for definition of REENTRANT

// Public Function Prototypes

bool InitBeaconTestHarness(uint8_t Priority);
bool PostBeaconTestHarness(ES_Event_t ThisEvent);
ES_Event_t RunBeaconTestHarness(ES_Event_t ThisEvent);

#endif /* BeaconTestHarness */

