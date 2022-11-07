/****************************************************************************
Module
    MorseCaptureService.c

Revision
    1.0.1

Description
    A service to measure the lengths of Morse code signals.

Aaron Brown
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "SensorService.h"

// Hardware
#include <xc.h>
#include <sys/attribs.h>

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
//#include "ES_DeferRecall.h"
#include "ES_Port.h"
#include "terminal.h"

// HALs
#include "PIC32PortHAL.h"

// Other services
#include "RobotHSM.h"

/*----------------------------- Module Defines ----------------------------*/
// Hardware
#define BEACON_PORT _Port_B
#define BEACON_PIN _Pin_4
//#define TAPE_PORT_L _Port_A
//#define TAPE_PIN_L _Pin_1
//#define TAPE_PORT_R _Port_B
//#define TAPE_PIN_R _Pin_2

#define MC_TIMEOUT 0xFFFF
#define FALLING 0
#define RISING 1

// 50ns*16 = 1250 ticks/ms
#define FOUR_US 5
#define ONE_MS 1250
#define PERIOD_A 75*FOUR_US // 300 uS (3333 Hz) 
#define PERIOD_B 275*FOUR_US // 1100 uS (909 Hz)
#define PERIOD_TOL 3*FOUR_US

// For framework timers
#define TAPE_TIMEOUT 50 // 50 ms

/*---------------------------- Module Functions ---------------------------*/
static void InitInputCapture(void);
static void StartInputCapture(void);
static void StopInputCapture(void);

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static bool LastEdge;
static timer32_t LastTime;
static timer32_t ThisTime;
static uint16_t RolloverCounter;
static uint32_t Period;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
 InitSensorService

 Parameters
 uint8_t : the priorty of this service

 Returns
 bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     Aaron Brown, 02/15/2022
****************************************************************************/
bool InitSensorService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  
  // Configure beacon pin as digital input
  if (!PortSetup_ConfigureDigitalInputs(BEACON_PORT, BEACON_PIN)) return false;
//  if (!PortSetup_ConfigureDigitalInputs(TAPE_PORT_L, TAPE_PIN_L)) return false;
//  if (!PortSetup_ConfigureDigitalInputs(TAPE_PORT_R, TAPE_PIN_R)) return false; 
  
  // Map input capture 4 to pin RB4
  IC4R = 0b0010;
  
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
 PostSensorService

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
 Aaron Brown, 02/15/2022
****************************************************************************/
bool PostSensorService(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
 RunSensorService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
 Aaron Brown, 02/15/2022
****************************************************************************/
ES_Event_t RunSensorService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
    switch (ThisEvent.EventType)
    {
        case ES_INIT:
        {
            printf("\rES_INIT received in Sensor Service\r\n");
            // Initialize input capture and timer
            InitInputCapture();
        }
        break;

        case SENSE_START_BEACON_IC:
        {
            StartInputCapture();
        }
        break;

        case SENSE_STOP_BEACON_IC:
        {
            StopInputCapture();
        }

      default:
      {}
      break;
    }

    return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
static void InitInputCapture(void)
{
    // Reset static variables
    LastEdge = FALLING;
    LastTime.Time = 0;
    RolloverCounter = 0;
    Period = 0;
    
    // Make sure input capture is disabled before configuring
    IC4CONbits.ON = 0;
    // Make sure that timer 2 is disabled before configuring
    T2CONbits.ON = 0;
    
    /************************************************
     * Set up input capture 4, IC4 (dedicate timer 2)
     ************************************************/
    // Select timer 2
    IC4CONbits.ICTMR = 1;
    // Capture rising edge first
    IC4CONbits.FEDGE = 1;
    // Operate in 16 bit mode
    IC4CONbits.C32 = 0;
    // Interrupt on every capture event
    IC4CONbits.ICI = 0;
    // Configure edge detect mode
    IC4CONbits.ICM = 0b010; // every falling edge
    // Make sure that we are set up for multiple interrupt vectors
    INTCONbits.MVEC = 1;
    // Enable the local input capture interrupt
    IEC0SET = _IEC0_IC4IE_MASK;
    // Set the interrupt priority for the input capture to level 7
    IPC4bits.IC4IP = 7;
    
    /*****************
     * Set up timer 2
     ****************/
    // Choose the internal clock as the source
    T2CONbits.TCS = 0;
    // Disable gated time mode
    T2CONbits.TGATE = 0;
    // Set up the pre-scale to divide by 16
    T2CONbits.TCKPS = 0b100;
    // Set timeout to specified period
    PR2 = MC_TIMEOUT;
    // Enable local timeout interrupt
    IEC0SET = _IEC0_T2IE_MASK;
    // Set the interrupt priority for the timer to level 6
    IPC2bits.T2IP = 6;
    
    // Make sure interrupts are enabled globally
    __builtin_enable_interrupts();
}

static void StartInputCapture(void)
{
    // Reset static variables
    LastEdge = FALLING;
    LastTime.Time = 0;
    RolloverCounter = 0;
    Period = 0;
    
    // Clear any pending flags
    IFS0CLR = _IEC0_IC4IE_MASK;
    IFS0CLR = _IFS0_T2IF_MASK;
    // Zero the timer count
    TMR2 = 0;
    // Enable the timer
    T2CONbits.ON = 1;
    // Enable the input capture
    IC4CONbits.ON = 1;
}

static void StopInputCapture(void)
{
    // Clear any pending flags
    IFS0CLR = _IEC0_IC4IE_MASK;
    IFS0CLR = _IFS0_T2IF_MASK;
    // Zero the timer count
    TMR2 = 0;
    // Disable the timer
    T2CONbits.ON = 0;
    // Disable the input capture
    IC4CONbits.ON = 0;
}

void __ISR(_INPUT_CAPTURE_4_VECTOR, IPL7SOFT) IC4ISR(void)
{
    static uint16_t CapturedTime; // static for speed
    do
    {
        CapturedTime = (uint16_t) IC4BUF; // Grab the captured time
        if (IFS0bits.T2IF == 1 && CapturedTime < 0x8000)
        {
            // timer 2 flag pending and time is after rollover
            ++RolloverCounter; // increment rollover counter
            IFS0CLR = _IFS0_T2IF_MASK; // clear the rollover interrupt
        }
        ThisTime.ByBytes[0] = CapturedTime;
        ThisTime.ByBytes[1] = RolloverCounter;
        Period = ThisTime.Time - LastTime.Time;
        LastTime.Time = ThisTime.Time; // update LastTime
        
        ES_Event_t NewEvent;
        if (Period > PERIOD_A-PERIOD_TOL && Period < PERIOD_A+PERIOD_TOL)
        {
            NewEvent.EventType = EV_BEACON_FOUND_A;
            PostRobotSM(NewEvent);
        }
        else if (Period > PERIOD_B-PERIOD_TOL && Period < PERIOD_B+PERIOD_TOL)
        {
            NewEvent.EventType = EV_BEACON_FOUND_B;
            PostRobotSM(NewEvent);
        }
    } while (IC4CONbits.ICBNE != 0); // until we have pulled all of the captures
    // Clear the capture interrupt
    IFS0CLR = _IFS0_IC4IF_MASK;
}
void __ISR(_TIMER_2_VECTOR, IPL6SOFT) Timer2ISR(void)
{
    // Disable interrupts globally
    __builtin_disable_interrupts();
    if (IFS0bits.T2IF == 1)
    {
        ++RolloverCounter; // increment the rollover counter
        IFS0CLR = _IFS0_T2IF_MASK; // clear the rollover interrupt
    }
    // Enable interrupts globally
    __builtin_enable_interrupts();
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

