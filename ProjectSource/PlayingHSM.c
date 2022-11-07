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
#include "PlayingHSM.h"
#include "SensorService.h"
#include "LeaderSPI.h"
#include "commdefs.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE MOVING_FWD
#define ONE_SEC 1000 // for framework timers
#define MOVEMENT_TIMEOUT 2500
#define SHOOTING_TIMEOUT 10*ONE_SEC
#define RELOADING_TIMEOUT 5*ONE_SEC
//#define FWD_DIST_0 200
//#define REV_DIST_0 220
//#define FWD_DIST 200
//#define REV_DIST 200

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringMovingFwdState(ES_Event_t Event);
static ES_Event_t DuringShootingState(ES_Event_t Event);
static ES_Event_t DuringMovingRevState(ES_Event_t Event);
static ES_Event_t DuringReloadingState(ES_Event_t Event);
static ES_Event_t DuringAligningShotState(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static PlayingState_t CurrentState;
static uint8_t Team = 0;
static uint8_t NumCycles = 0;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
 RunPlayingSM

 Parameters
   ES_Event_t: the event to process

 Returns
   ES_Event_t: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/
ES_Event_t RunPlayingSM(ES_Event_t CurrentEvent)
{
   bool MakeTransition = false;/* are we making a state transition? */
   PlayingState_t NextState = CurrentState;
   ES_Event_t EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event_t ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch (CurrentState)
   {
        case MOVING_FWD:       // If current state is state one
        {
            ReturnEvent = CurrentEvent = DuringMovingFwdState(CurrentEvent);
            //process any events
            if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
            {
                switch (CurrentEvent.EventType)
                {
                    case PLY_ENTERED_FIELD:
                    {
//                        printf("\r\nPLY_ENTERED_FIELD received in PlayingHSM\r\n");
//                        printf("\rMOVING_FWD -> SHOOTING\r\n\n");
                        NextState = ALIGNING_SHOT;
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY_HISTORY;
                    }
                    break;
                }
            }
        }
        break;
        
        case ALIGNING_SHOT:
        {
            ReturnEvent = CurrentEvent = DuringAligningShotState(CurrentEvent);
            if (CurrentEvent.EventType != ES_NO_EVENT)
            {
                switch (CurrentEvent.EventType)
                {
                    case EV_ALIGN_COMPLETE:
                    {
                        NextState = SHOOTING;
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY_HISTORY;
                    }
                    break;
                }
            }
        }
        break;
       
        case SHOOTING:
        {
            ReturnEvent = CurrentEvent = DuringShootingState(CurrentEvent);
            //process any events
            if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
            {
                switch (CurrentEvent.EventType)
                {   
                    case PLY_FIRE_COMPLETE:
                    {
//                        printf("\r\nPLY_FIRE_COMPLETE received in PlayingHSM\r\n");
//                        printf("\rSHOOTING -> MOVING_REV\r\n\n");
                        NextState = MOVING_REV;
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY_HISTORY;
                    }
                }
            }
        }
        break;
        
        case MOVING_REV:
        {
            ReturnEvent = CurrentEvent = DuringMovingRevState(CurrentEvent);
            if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
            {
                switch (CurrentEvent.EventType)
                {
                    case PLY_ENTERED_RELOAD:
                    {
//                        printf("\r\nPLY_ENTERED_RELOAD received in PlayingHSM\r\n");
//                        printf("\rMOVING_REV -> RELOADING\r\n\n");
                        NumCycles++;
                        NextState = RELOADING;
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY_HISTORY;
                    }
                    break;
                }
            }
        }
        break;
        
        case RELOADING:
        {
            ReturnEvent = CurrentEvent = DuringReloadingState(CurrentEvent);
            if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
            {
                switch (CurrentEvent.EventType)
                {
                    case PLY_RELOAD_BUTTON_PRESSED:
                    {
//                        printf("\r\nPLY_RELOAD_BUTTON_PRESSED received in PlayingHSM\r\n");
//                        printf("\rRELOADING -> MOVING_FWD\r\n\n");
                        NextState = MOVING_FWD;
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY_HISTORY;
                    }
                    break;
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
        RunPlayingSM(CurrentEvent);

        CurrentState = NextState; //Modify state variable

        //   Execute entry function for new state
        // this defaults to ES_ENTRY
        RunPlayingSM(EntryEventKind);
      }
      return(ReturnEvent);
}
/****************************************************************************
 Function
     StartTemplateSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartPlayingSM ( ES_Event_t CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   Team = CurrentEvent.EventParam;
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunPlayingSM(CurrentEvent);
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
PlayingState_t QueryPlayingSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringMovingFwdState(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        ES_Timer_InitTimer(MOVEMENT_TIMER, MOVEMENT_TIMEOUT);
        
        if (NumCycles == 0)
        {
            ES_Event_t CommEvent;
            CommEvent.EventType = COMM_FWD;
            CommEvent.EventParam = DRIVE_FWD_0;
            PostLeaderSPI(CommEvent);
        }
        else
        {
            ES_Event_t CommEvent;
            CommEvent.EventType = COMM_FWD;
            CommEvent.EventParam = DRIVE_FWD;
            PostLeaderSPI(CommEvent);
        }
        
//        ES_Event_t CommEvent;
//        CommEvent.EventType = COMM_FWD;
//        CommEvent.EventParam = DRIVE_FWD;
//        PostLeaderSPI(CommEvent);
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
        ES_Event_t CommEvent;
        CommEvent.EventType = COMM_STOP;
        CommEvent.EventParam = STOP;
        PostLeaderSPI(CommEvent);
    }
    else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
        switch (Event.EventType)
        {   
            case ES_TIMEOUT:
            {
                if (Event.EventParam == MOVEMENT_TIMER)
                {
                    // beacon not found, move to realign state
//                    printf("\r\nES_TIMEOUT received in PlayingSM\r\n");
                    ReturnEvent.EventType = PLY_ENTERED_FIELD;
                }
            }
        }
        
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event_t DuringShootingState(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        ES_Event_t CommEvent;
        CommEvent.EventType = COMM_FIRE;
        CommEvent.EventParam = FIRE;
        PostLeaderSPI(CommEvent);
        
//        ONLY INIT TIMER WHEN NOT USING THE FIRE UPDATE (RA0 LINE) EVENT CHECKER
//        ES_Timer_InitTimer(SHOOTING_TIMER, SHOOTING_TIMEOUT);
       
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
        switch (Event.EventType)
        {   
            case ES_TIMEOUT:
            {
                if (Event.EventParam == SHOOTING_TIMER)
                {
//                    printf("\r\nES_TIMEOUT received in PlayingSM\r\n");
                    ReturnEvent.EventType = PLY_FIRE_COMPLETE;
                }
            }
        }
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event_t DuringMovingRevState(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event;

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        ES_Timer_InitTimer(MOVEMENT_TIMER, MOVEMENT_TIMEOUT);
        
        if (NumCycles == 0)
        {
            ES_Event_t CommEvent;
            CommEvent.EventType = COMM_REV;
            CommEvent.EventParam = DRIVE_REV_0;
            PostLeaderSPI(CommEvent);
        }
        else
        {
            ES_Event_t CommEvent;
            CommEvent.EventType = COMM_REV;
            CommEvent.EventParam = DRIVE_REV;
            PostLeaderSPI(CommEvent);
        }
        
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
        ES_Event_t CommEvent;
        CommEvent.EventType = COMM_STOP;
        CommEvent.EventParam = STOP;
        PostLeaderSPI(CommEvent);
    }
    else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
        switch (Event.EventType)
        {   
            case ES_TIMEOUT:
            {
                if (Event.EventParam == MOVEMENT_TIMER)
                {
//                    printf("\r\nES_TIMEOUT received in PlayingSM\r\n");
                    ReturnEvent.EventType = PLY_ENTERED_RELOAD;
                }
            }
        }
        
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event_t DuringReloadingState(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event;

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
        
//        ONLY INIT TIMER WHEN NOT USING THE FIRE UPDATE (RA0 LINE) EVENT CHECKER
//        ES_Timer_InitTimer(RELOADING_TIMER, RELOADING_TIMEOUT);
    }
    else if ( Event.EventType == ES_EXIT )
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
        switch (Event.EventType)
        {   
            case ES_TIMEOUT:
            {
                if (Event.EventParam == RELOADING_TIMER)
                {
//                    printf("\r\nES_TIMEOUT received in PlayingSM\r\n");
                    ReturnEvent.EventType = PLY_RELOAD_BUTTON_PRESSED;
                }
            }
        }
        
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event_t DuringAligningShotState(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
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
        
        ES_Event_t CommEvent;
        CommEvent.EventType = COMM_STOP;
        CommEvent.EventParam = STOP;
        PostLeaderSPI(CommEvent);
      
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
            if (TEAM_A == Team)
            {
                ReturnEvent.EventType = EV_ALIGN_COMPLETE;
            }
        }
        else if (Event.EventType == EV_BEACON_FOUND_B)
        {
            if (TEAM_B == Team)
            {
                ReturnEvent.EventType = EV_ALIGN_COMPLETE;
            }
        }
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}
