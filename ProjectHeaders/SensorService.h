/****************************************************************************

  Header file for Test Harness Service0
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef SensorService_H
#define SensorService_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Port.h"                // needed for definition of REENTRANT

typedef union {
    uint32_t Time;
    uint16_t ByBytes[2];
} timer32_t; // 0 has LSB, 1 has MSB

// Public Function Prototypes
bool InitSensorService(uint8_t Priority);
bool PostSensorService(ES_Event_t ThisEvent);
ES_Event_t RunSensorService(ES_Event_t ThisEvent);

#endif /* SensorService_H */

