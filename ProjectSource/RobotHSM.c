/****************************************************************************
 Module
   RobotHSM.c

 Revision
   1.0

 Description
 High level robot logic

 Notes
 * Drivetrain Commands
 *      0xD1: STOP
 *      0xD2: ROT_CCW
 *      0xD3: ROT_CW
 *      0xD4: DRIVE_FWD
 *      0xD5: DRIVE_REV
 *      0xD6: TEAM_A
 *      0xD7: TEAM_B
 * Launcher Commands
 *      0xF1: FLAG_UP
 *      0xF2: FLAG_DOWN
 *      0xF3: FIRE
 History
 When           Who     What/Why
 -------------- ---     --------
 02/16/22 11:39 ahb     Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This Module
#include "RobotHSM.h"

// Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "terminal.h"

// HAL
#include "PIC32PortHAL.h"

// Project Headers
#include "commdefs.h"
#include "IdentifyingHSM.h"
#include "PlayingHSM.h"
#include "LeaderSPI.h"

/*----------------------------- Module Defines ----------------------------*/
#define START_BUTTON_PORT _Port_A
#define START_BUTTON_PIN _Pin_2
#define START_BUTTON PORTAbits.RA2

#define FIRE_UPDATE_PORT _Port_B
#define FIRE_UPDATE_PIN _Pin_2
#define FIRE_UPDATE PORTBbits.RB2

#define MOVEMENT_UPDATE_PORT _Port_A
#define MOVEMENT_UPDATE_PIN _Pin_1
#define MOVEMENT_UPDATE PORTAbits.RA1

#define ONE_SEC 1000 // for framework timers
#define ONE_MIN 60*ONE_SEC
/*---------------------------- Module Functions ---------------------------*/
static ES_Event_t DuringWaitingState(ES_Event_t Event);
static ES_Event_t DuringIdentifyingState(ES_Event_t Event);
static ES_Event_t DuringPlayingState(ES_Event_t Event);
static ES_Event_t DuringGameOverState(ES_Event_t Event);
static void HandleGameTimer(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static RobotState_t CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

static uint8_t CurrentStartButtonState;
static uint8_t LastStartButtonState;
static uint8_t CurrentFireUpdateState;
static uint8_t LastFireUpdateState;
static uint8_t CurrentMovementUpdateState;
static uint8_t LastMovementUpdateState;
static uint8_t Game_Minute;
static uint8_t Game_Second;
static bool IsPlaying = false;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitRobotSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     boolean, False if error in initialization, True otherwise

 Description
     Saves away the priority,  and starts
     the top level state machine
 Notes

 Author
 Aaron Brown
****************************************************************************/
bool InitRobotSM (uint8_t Priority)
{   
    ES_Event_t ThisEvent;

    // Save our priority
    MyPriority = Priority;  
    
    if (!PortSetup_ConfigurePullUps(START_BUTTON_PORT,START_BUTTON_PIN)) return false;
    if (!PortSetup_ConfigureDigitalInputs(FIRE_UPDATE_PORT,FIRE_UPDATE_PIN)) return false;
    if (!PortSetup_ConfigureDigitalInputs(MOVEMENT_UPDATE_PORT,MOVEMENT_UPDATE_PIN)) return false;
    CurrentStartButtonState = START_BUTTON;
    LastStartButtonState = CurrentStartButtonState;
    CurrentFireUpdateState = FIRE_UPDATE;
    LastFireUpdateState = CurrentFireUpdateState;
    CurrentMovementUpdateState = MOVEMENT_UPDATE;
    LastMovementUpdateState = CurrentMovementUpdateState;
    
    // For game timer
    Game_Minute = 0;
    Game_Second = 0;

    // Start the Robot State machine
    ThisEvent.EventType = ES_ENTRY;
    StartRobotSM(ThisEvent);

    return true;
}

/****************************************************************************
 Function
    PostRobotSM

 Parameters
    ES_Event_t ThisEvent , the event to post to the queue

 Returns
    boolean False if the post operation failed, True otherwise

 Description
    Posts an event to this state machine's queue
 Notes
    N/A
 Author
    Aaron Brown
****************************************************************************/
bool PostRobotSM( ES_Event_t ThisEvent )
{
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
 RunRobotSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   the run function for the top level state machine 
 Notes
   uses nested switch/case to implement the machine.
 Author
 Aaron Brown
****************************************************************************/
ES_Event_t RunRobotSM( ES_Event_t CurrentEvent )
{
    bool MakeTransition = false;/* are we making a state transition? */
    RobotState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
    ES_Event_t ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

    switch (CurrentState)
    {   
        case WAITING:
        {
            CurrentEvent = DuringWaitingState(CurrentEvent);
            if (CurrentEvent.EventType != ES_NO_EVENT)
            {
                switch (CurrentEvent.EventType)
                {
                    case EV_START_BUTTON_PRESSED:
                    {
                        printf("\r\nEV_START_BUTTON_PRESSED received in RobotHSM\r\n");
                        printf("\rWAITING -> IDENTIFYING\r\n\n");
                        NextState = IDENTIFYING;
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY_HISTORY;
                    }
                    break;
                }
            }
        }
        break;
        
        case IDENTIFYING:
        {
            CurrentEvent = DuringIdentifyingState(CurrentEvent);
            if ( CurrentEvent.EventType != ES_NO_EVENT )
            {
                switch (CurrentEvent.EventType)
                {
                    case EV_PLAY_BALL:
                    {
                        printf("\r\nALIGN COMPLETE\r\n");
                        printf("\rIDENTIFYING -> PLAYING\r\n\n");
                        NextState = PLAYING;
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY_HISTORY;
                        EntryEventKind.EventParam = CurrentEvent.EventParam;
                    }
                    break;
                }
            }
        }
        break;
        
        case PLAYING:
        {
            CurrentEvent = DuringPlayingState(CurrentEvent);
            //process any events
            if ( CurrentEvent.EventType != ES_NO_EVENT )
            {
                switch (CurrentEvent.EventType)
                {
                    case EV_GAME_OVER:
                    {
                        NextState = GAME_OVER;
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY_HISTORY;
                        EntryEventKind.EventParam = CurrentEvent.EventParam;
                    }
                    break;
                }
                // do something
            }
        }
        break;
        
        case GAME_OVER:
        {
            CurrentEvent = DuringGameOverState(CurrentEvent);
        }
        break;
     }
     //   If we are making a state transition
     if (MakeTransition == true)
     {
        //   Execute exit function for current state
        CurrentEvent.EventType = ES_EXIT;
        RunRobotSM(CurrentEvent);

        CurrentState = NextState; //Modify state variable

        // Execute entry function for new state
        // this defaults to ES_ENTRY
        RunRobotSM(EntryEventKind);
      }
    // in the absence of an error the top level state machine should
    // always return ES_NO_EVENT, which we initialized at the top of func
    return(ReturnEvent);
}
/****************************************************************************
 Function
     StartRobotSM

 Parameters
     ES_Event CurrentEvent

 Returns
     nothing

 Description
     Does any required initialization for this state machine
 Notes

 Author
    Aaron Brown
****************************************************************************/
void StartRobotSM (ES_Event_t CurrentEvent)
{
    // if there is more than 1 state to the top level machine you will need 
    // to initialize the state variable
    CurrentState = WAITING;
    // now we need to let the Run function init the lower level state machines
    // use LocalEvent to keep the compiler from complaining about unused var
    RunRobotSM(CurrentEvent);
    return;
}

/****************************************************************************
 Function
     QueryTopHSMTemplateSM

 Parameters
     None

 Returns
     MasterState_t  The current state of the Top Level Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/05/22, 10:30AM
****************************************************************************/
RobotState_t  QueryRobotSM ( void )
{
   return(CurrentState);
}

/*
 Event Checkers
 */
bool CheckStartButtonPressed(void)
{
    CurrentStartButtonState = START_BUTTON;
    if (CurrentState == WAITING && CurrentStartButtonState != LastStartButtonState)
    {
        ES_Event_t ThisEvent;
        ThisEvent.EventType   = EV_START_BUTTON_PRESSED;
        ThisEvent.EventParam  = 0;
        PostRobotSM(ThisEvent);
//        LastStartButtonState = CurrentStartButtonState;
        return true;
    }
    return false;
}

bool CheckFireUpdate(void)
{
//    return false;
    CurrentFireUpdateState = FIRE_UPDATE;
//    printf("\r%d%d\r\n",CurrentFireUpdateState,LastFireUpdateState);
    if (CurrentFireUpdateState != LastFireUpdateState)
    {
        PlayingState_t PlayingState = QueryPlayingSM();
//        tt
        LastFireUpdateState = CurrentFireUpdateState;
        
        if (PlayingState == SHOOTING && CurrentFireUpdateState == 1)
        {
            printf("SHOOTING DONE\r\n");
//            LastFireUpdateState = CurrentFireUpdateState;
            ES_Event_t ThisEvent;
            ThisEvent.EventType   = PLY_FIRE_COMPLETE;
            ThisEvent.EventParam  = 0;
            PostRobotSM(ThisEvent);
            return true;
        }
        else if (PlayingState == RELOADING && CurrentFireUpdateState == 0)
        {
            printf("RELOAD DONE\r\n");
//            LastFireUpdateState = CurrentFireUpdateState;
            ES_Event_t ThisEvent;
            ThisEvent.EventType   = PLY_RELOAD_BUTTON_PRESSED;
            ThisEvent.EventParam  = 0;
            PostRobotSM(ThisEvent);
            return true;
        }
    }
  return false;
}

bool CheckMovementUpdate(void)
{
    return false;
    CurrentMovementUpdateState = MOVEMENT_UPDATE;
    if (CurrentState == PLAYING && CurrentMovementUpdateState != LastMovementUpdateState)
    {
        PlayingState_t PlayingState = QueryPlayingSM();
        LastMovementUpdateState = CurrentMovementUpdateState;
        if (PlayingState == MOVING_FWD)
        {
            ES_Event_t ThisEvent;
            ThisEvent.EventType   = PLY_ENTERED_FIELD;
            ThisEvent.EventParam  = 0;
            PostRobotSM(ThisEvent);
            return true;
        }
        else if (PlayingState == MOVING_REV)
        {
            ES_Event_t ThisEvent;
            ThisEvent.EventType   = PLY_ENTERED_RELOAD;
            ThisEvent.EventParam  = 0;
            PostRobotSM(ThisEvent);
            return true;
        }
    }
  return false;
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringWaitingState( ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
        printf("\r***GAME TIMER STARTED***\r\n");
        ES_Timer_InitTimer(GAME_TIMER, ONE_MIN);
        
        IsPlaying = true;
        
        ES_Event_t CommEvent;
        CommEvent.EventType = COMM_FLAG_UP;
        CommEvent.EventParam = FLAG_UP;
        PostLeaderSPI(CommEvent);
    }
    else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
//        if (ReturnEvent.EventType == ES_TIMEOUT)
//        {
//            if (ReturnEvent.EventParam == GAME_TIMER)
//            {
//                HandleGameTimer();
//                ReturnEvent.EventType = ES_NO_EVENT;
//            }
//        }
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event_t DuringIdentifyingState( ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartIdentifyingSM(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunIdentifyingSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
    }
    else
    {
        // run any lower level state machine
        ReturnEvent = RunIdentifyingSM(Event);
        
        if (ReturnEvent.EventType == ES_TIMEOUT)
        {
            if (ReturnEvent.EventParam == GAME_TIMER)
            {
                HandleGameTimer();
                ReturnEvent.EventType = ES_NO_EVENT;
            }
        }
    }
    return(ReturnEvent);
}

static ES_Event_t DuringPlayingState(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartPlayingSM(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if (Event.EventType == ES_EXIT)
    {
        // on exit, give the lower levels a chance to clean up first
        RunPlayingSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
//        printf("\r***TEAM COMM SENT***\r\n");
    }
    else
    {
        // run any lower level state machine
        ReturnEvent = RunPlayingSM(Event);
        if (ReturnEvent.EventType == ES_TIMEOUT)
        {
            if (ReturnEvent.EventParam == GAME_TIMER)
            {
                HandleGameTimer();
                ReturnEvent.EventType = ES_NO_EVENT;
            }
        }
    }
    return(ReturnEvent);
}

static ES_Event_t DuringGameOverState(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        // stop the motors
        ES_Event_t CommEvent;
        CommEvent.EventType = COMM_STOP;
        CommEvent.EventParam = STOP;
        PostLeaderSPI(CommEvent);
        
        // Signal game over
        CommEvent.EventType = COMM_FLAG_DOWN;
        CommEvent.EventParam = FLAG_DOWN;
        PostLeaderSPI(CommEvent);
        // after that start any lower level machines that run in this state
        
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if (Event.EventType == ES_EXIT)
    {
        // on exit, give the lower levels a chance to clean up first
        
        // repeat for any concurrently running state machines
        // now do any local exit functionality
//        printf("\r***TEAM COMM SENT***\r\n");
    }
    else
    {
        // run any lower level state machine
        
    }
    return(ReturnEvent);
}

static void HandleGameTimer()
{
    if (Game_Minute < 1)
    { 
        Game_Minute++;
        printf("\r MINUTE: %d \r\n", Game_Minute);
        ES_Timer_InitTimer(GAME_TIMER, ONE_MIN);
    }
    else if (Game_Second < 18)
    {
        printf("\r TWO MINUTES AND %d SECONDS \r\n", Game_Second);
        Game_Second++;
        ES_Timer_InitTimer(GAME_TIMER, ONE_SEC);
    }
    else
    {
        IsPlaying = false;
        
        ES_Event_t NewEvent;
        NewEvent.EventType = EV_GAME_OVER;
        PostRobotSM(NewEvent);
    }
}