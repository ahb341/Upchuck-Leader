/****************************************************************************
 * Module
 * IdentifyingHSM
 * Author
 * Aaron Brown
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Hardware
#include <xc.h>
#include <sys/attribs.h>

// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "IdentifyingHSM.h"
#include "SensorService.h"
#include "LeaderSPI.h"
#include "commdefs.h"

/*----------------------------- Module Defines ----------------------------*/
#define ENTRY_STATE ALIGN
#define ONE_SEC 1000 // for framework timers
#define BEACON_TIMEOUT 20*ONE_SEC
#define STOP_TIMEOUT 1*ONE_SEC

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringAlignState(ES_Event_t Event);
static ES_Event_t DuringIdlingState(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static IdentifyingState_t CurrentState;
static uint8_t Team = 0; // by default

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
 RunIdentifyingSM

 Parameters
   ES_Event_t: the event to process

 Returns
   ES_Event_t: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   Aaron Brown, 2/17/22, 19:57
****************************************************************************/
ES_Event_t RunIdentifyingSM(ES_Event_t CurrentEvent)
{
   bool MakeTransition = false;/* are we making a state transition? */
   IdentifyingState_t NextState = CurrentState;
   ES_Event_t EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event_t ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch (CurrentState)
   {
        case ALIGN:
        {
            ReturnEvent = CurrentEvent = DuringAlignState(CurrentEvent);
            if (CurrentEvent.EventType != ES_NO_EVENT)
            {
                switch (CurrentEvent.EventType)
                {
                    case EV_ALIGN_COMPLETE :
                    {
                        Team = CurrentEvent.EventParam;
                        printf("\r\nEV_ALIGN_COMPLETE received in IdentifyingSM\r\n");
                        printf("\rALIGN -> IDLING\r\n\n");
                        NextState = IDLING;
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY_HISTORY;
                    }
                    break;
                }
            }
        }
        break;
        case IDLING :       // If current state is state one
        {
            // Execute During function for state one. ES_ENTRY & ES_EXIT are
            // processed here allow the lower level state machines to re-map
            // or consume the event
            ReturnEvent = CurrentEvent = DuringIdlingState(CurrentEvent);
            //process any events
            if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
            {
                switch (CurrentEvent.EventType)
                {
                    // N/a
                }
            }
        }
        break;
   }
     //   If we are making a state transition
     if (MakeTransition == true)
     {
        //   Execute exit function for current state
        CurrentEvent.EventType = ES_EXIT;
        RunIdentifyingSM(CurrentEvent);

        CurrentState = NextState; //Modify state variable

        //   Execute entry function for new state
        // this defaults to ES_ENTRY
        RunIdentifyingSM(EntryEventKind);
      }
      return(ReturnEvent);
}
/****************************************************************************
 Function
 StartIdentifyingSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     Aaron Brown, 2/17/22, 20:06
****************************************************************************/
void StartIdentifyingSM (ES_Event_t CurrentEvent)
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunIdentifyingSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
IdentifyingState_t QueryIdentifyingSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringAlignState(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        printf("\r ES_ENTRY RECEIVED IN ALIGN\r\n");
        // FOR CHECKOFF ONLY
        ES_Event_t NewEvent;
        NewEvent.EventType = SENSE_START_BEACON_IC;
        PostSensorService(NewEvent);
        // ***************
        
        ES_Event_t CommEvent;
        CommEvent.EventType = COMM_ROT_CCW;
        CommEvent.EventParam = ROT_CCW;
        PostLeaderSPI(CommEvent);
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if (Event.EventType == ES_EXIT)
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
        
        // FOR CHECKOFF ONLY
        ES_Event_t NewEvent;
        NewEvent.EventType = SENSE_STOP_BEACON_IC;
        PostSensorService(NewEvent);
        
      
    }
    else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
        if (Event.EventType == EV_BEACON_FOUND_A)
        {
            printf("\rTEAM A\r\n");
            ReturnEvent.EventType = EV_ALIGN_COMPLETE;
            ReturnEvent.EventParam = TEAM_A;
            
            ES_Event_t CommEvent;
            CommEvent.EventType = COMM_TEAM_FOUND;
            CommEvent.EventParam = TEAM_A;
            PostLeaderSPI(CommEvent);
        }
        else if (Event.EventType == EV_BEACON_FOUND_B)
        {
            printf("\rTEAM B\r\n");
            ReturnEvent.EventType = EV_ALIGN_COMPLETE;
            ReturnEvent.EventParam = TEAM_B;
            
            ES_Event_t CommEvent;
            CommEvent.EventType = COMM_TEAM_FOUND;
            CommEvent.EventParam = TEAM_B;
            PostLeaderSPI(CommEvent);
        
        }
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event_t DuringIdlingState(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        // FOR CHECKOFF ONLY
        ES_Event_t CommEvent;
        CommEvent.EventType = COMM_STOP;
        CommEvent.EventParam = STOP;
        PostLeaderSPI(CommEvent);
        ES_Timer_InitTimer(STOP_TIMER, STOP_TIMEOUT);
        
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if (Event.EventType == ES_EXIT)
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }
    else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
        if (Event.EventType == ES_TIMEOUT)
        {
            if (Event.EventParam == STOP_TIMER)
            {
                printf("\r\nSTOP COMPLETE\r\n");
                ReturnEvent.EventType = EV_PLAY_BALL;
                ReturnEvent.EventParam = Team;
            }
        }
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}