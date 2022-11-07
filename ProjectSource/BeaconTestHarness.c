/****************************************************************************
 Module
   BeaconTestHarness.c

 Revision
   1.0.1

 Description
 A module to operate the beacon transmitter

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
 02/17/22 19:04 ahb     beacon test harness code
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "BeaconTestHarness.h"

// Hardware
#include <xc.h>
#include <sys/attribs.h> // for ISR macors

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Port.h"
#include "terminal.h"
#include "bitdefs.h"

// C Standard
#include <stdio.h>

// HALs
#include "PIC32PortHAL.h"

/*----------------------------- Module Defines ----------------------------*/
// Prescale
#define DIVBY8 0b011

// TICS_PER_MS assumes a 20MHz PBClk / 8 = 2.5MHz clock rate
#define TWO_US 5
#define TICS_PER_MS 2500
#define PWM_PERIOD_A 150*TWO_US // 300 us period
#define PWM_PERIOD_B 550*TWO_US // 1100 us period
#define PWM_PERIOD PWM_PERIOD_A

// Interrupt priorities
#define PWM_TIMER_PRIORITY 5

/*---------------------------- Module Functions ---------------------------*/
static void PrintBeaconTestHarness(void);
static void SetupPWM(void);

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
 InitBeaconTestHarness

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     Aaron Brown, 01/25/22
****************************************************************************/
bool InitBeaconTestHarness(uint8_t Priority)
{
    MyPriority = Priority;
    PrintBeaconTestHarness();
    if (!PortSetup_ConfigureDigitalOutputs(_Port_B,_Pin_5)) return false; // Out
//    PORTBbits.RB5 = 1;
    SetupPWM(); // InitPWM (Timer 3))

    // post the initial transition event
    ES_Event_t ThisEvent;
    ThisEvent.EventType = ES_INIT;
    if (!ES_PostToService(MyPriority, ThisEvent)) return false;
    return true;
}

/****************************************************************************
 Function
 PostBeaconTestHarness

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     Aaron Brown, 01/25/22
****************************************************************************/
bool PostBeaconTestHarness(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
 RunBeaconTestHarness

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   Aaron Brown, 01/25/22
****************************************************************************/
ES_Event_t RunBeaconTestHarness(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    switch (ThisEvent.EventType)
    {
        case ES_INIT:
        {
            // Set duty cycle to 100 to start the beacon
            printf("\rES_INIT received in Beacon Test Harness\r\n");
            uint16_t Cmd_DC = 50;
            uint32_t Cmd = ((float)Cmd_DC/100.0)*(PWM_PERIOD+1.0);
            OC2RS = Cmd;
        }
        break;  
        
        default:
        {}
        break;
    }

    return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
static void PrintBeaconTestHarness()
{
    printf("\rStarting Beacon Test Harness\r\n");
}

static void SetupPWM()
{
    // ************* PWM SETUP **************
    // *** Configure RB5 for PWM on OC2 ***
    // **************************************
//    if (!PortSetup_ConfigureDigitalOutputs(_Port_B,_Pin_15)) return false; // PWM pin
    
    // Turn off Timer 3
    T3CONbits.ON = 0;
    
    // Clear flag
    IFS0bits.T3IF = 0;
    
    // base Timer3 on PBClk/8
    T3CONbits.TCS = 0;  // use PBClk as clock source 
    T3CONbits.TGATE = 0;
    T3CONbits.TCKPS = DIVBY8;  // divide by 8 --> 50ns*8 = 400 nS
    
    // Set the PWM period by writing to the timer3 period register
    PR3 = PWM_PERIOD;
    
    // turn off the OC system before making changes
    OC2CONbits.ON = 0;
    
    // Select timer 3
    OC2CONbits.OCTSEL = 1;
    
    // 16 bit compare
    OC2CONbits.OC32 = 0;
    
    // set PWM mode with no fault detect
    OC2CONbits.OCM = 0b110;
    
    // Set the PWM duty cycle by writing to the OCxRS register (0 by default)
    OC2RS = 0;
    
    // Write the OxCR register with the initial duty cycle
    OC2R = 0;
    
    // Map RB5 to OC2
    RPB5R = 0b0101;
    
    // Enable interrupts for the timer
    IEC0SET = _IEC0_T3IE_MASK;
    // Set the interrupt priority for the timer to level 5
    IPC3bits.T3IP = 5;
    
    // Turn ON the Output Compare module
    OC2CONbits.ON = 1;
    
    // Turn ON the timer
    T3CONbits.ON = 1;
    
    __builtin_enable_interrupts();
}

/*------------------------------- ISRs -------------------------------*/
void __ISR(_TIMER_3_VECTOR, IPL5SOFT) Timer3ISR(void)
{
    // Disable interrupts globally
    __builtin_disable_interrupts();
    if (IFS0bits.T3IF == 1)
    {
        IFS0CLR = _IFS0_T3IF_MASK; // clear the interrupt
    }
    // Enable interrupts globally
    __builtin_enable_interrupts();
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

