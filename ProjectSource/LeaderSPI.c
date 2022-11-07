/****************************************************************************
 Module
   LeaderSPI.c

 Revision
   1.0.1

 Description
   This is a template file for implementing flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include <xc.h>
#include <sys/attribs.h>
#include <proc/p32mx170f256b.h>

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"

#include "LeaderSPI.h"
#include "PIC32PortHAL.h"

/*----------------------------- Module Defines ----------------------------*/
#define CS_DT BIT12HI; // drivetrain chip select
#define CS_LA BIT15HI; // launcher chip select
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static void ConfigureLeaderSPI(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static LeaderSPIState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static ES_Event_t DeferralQueue[3+1];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitLeaderSPI

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitLeaderSPI(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  
  ES_InitDeferralQueueWith(DeferralQueue,ARRAY_SIZE(DeferralQueue));
  
  /////////////////////////
  INTCONbits.MVEC = 1;
  
  __builtin_disable_interrupts();
  
  if (!PortSetup_ConfigureDigitalOutputs(_Port_B, _Pin_12)) return false; // CS1
  if (!PortSetup_ConfigureDigitalOutputs(_Port_B, _Pin_15)) return false; // CS2
  
  LATBSET = BIT12HI | BIT15HI;    // Set both CS high
  
  ConfigureLeaderSPI();
  
  __builtin_enable_interrupts();
  ////////////////////////
  
  // put us into the Initial PseudoState
  CurrentState = InitPState;
  
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
     PostLeaderSPI

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostLeaderSPI(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunLeaderSPI

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunLeaderSPI(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
  switch (CurrentState)
  {
    case InitPState:        // If current state is initial Psedudo State
    {
      if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
      {
        // this is where you would put any actions associated with the
        // transition from the initial pseudo-state into the actual
        // initial state

        // now put the machine into the actual initial state
        CurrentState = Waiting;
        printf("\rES INIT RECEIVED IN SPI\r\n");
      }
    }
    break;
    
    case Waiting:
    {
        switch (ThisEvent.EventType)
        {
            case COMM_FLAG_UP:
            {
                printf("\r COMM_FLAG_UP---LeaderSPI---Waiting State\r\n");
                CurrentState = Sending;
                ThisEvent.EventType = COMM_LAUNCHER;
                PostLeaderSPI(ThisEvent);
            }
            break;
            
            case COMM_FLAG_DOWN:
            {
                printf("\r COMM_FLAG_DOWN---LeaderSPI---Waiting State\r\n");
                CurrentState = Sending;
                ThisEvent.EventType = COMM_LAUNCHER;
                PostLeaderSPI(ThisEvent);
            }
            break;
            
            case COMM_FIRE:
            {
                printf("\r COMM_FIRE---LeaderSPI---Waiting State\r\n");
                CurrentState = Sending;
                ThisEvent.EventType = COMM_LAUNCHER;
                PostLeaderSPI(ThisEvent);
            }
            break;
            
            case COMM_TEAM_FOUND:
            {
                printf("\r COMM_TEAM_FOUND---LeaderSPI---Waiting State\r\n");
                CurrentState = Sending;
                ThisEvent.EventType = COMM_DRIVETRAIN;
                PostLeaderSPI(ThisEvent);
            }
            break;
            
            case COMM_ROT_CCW:
            {
                printf("\r COMM_ROT_CCW---LeaderSPI---Waiting State\r\n");
                CurrentState = Sending;
                ThisEvent.EventType = COMM_DRIVETRAIN;
                PostLeaderSPI(ThisEvent);
            }
            break;
            
            case COMM_ROT_CW:
            {
                printf("\r COMM_ROT_CW---LeaderSPI---Waiting State\r\n");
                CurrentState = Sending;
                ThisEvent.EventType = COMM_DRIVETRAIN;
                PostLeaderSPI(ThisEvent);
            }
            break;
            
            case COMM_STOP:
            {
                printf("\r COMM_STOP---LeaderSPI---Waiting State\r\n");
                CurrentState = Sending;
                ThisEvent.EventType = COMM_DRIVETRAIN;
                PostLeaderSPI(ThisEvent);
            }
            break;
            
            case COMM_FWD:
            {
                printf("\r COMM_FWD---LeaderSPI---Waiting State\r\n");
                CurrentState = Sending;
                ThisEvent.EventType = COMM_DRIVETRAIN;
                PostLeaderSPI(ThisEvent);
            }
            break;
            
            case COMM_REV:
            {
                printf("\r COMM_REV---LeaderSPI---Waiting State\r\n");
                CurrentState = Sending;
                ThisEvent.EventType = COMM_DRIVETRAIN;
                PostLeaderSPI(ThisEvent);
            }
            break;
            
            default:
            break;
        }
    }
    break;
    
    case Sending:
    {
        switch (ThisEvent.EventType)
        {
            case COMM_DRIVETRAIN:
            {
                printf("\rSENDING SPI TO DRIVETRAIN\r\n");
                uint8_t Cmd = (uint8_t) ThisEvent.EventParam;
                LATBCLR = CS_DT;              // Set CS drivetrain low
                SPI1BUF = Cmd;
                //while(0 == SPI1STATbits.SRMT);
                
                IEC1bits.SPI1TXIE = 1;          // Enable transmit intrpt
            }
            break;
            
             case COMM_LAUNCHER:
            {
                uint8_t Cmd = (uint8_t) ThisEvent.EventParam;
                LATBCLR = CS_LA;              // Set CS launcher low
                SPI1BUF = Cmd;
                
                
                IEC1bits.SPI1TXIE = 1;          // Enable transmit intrpt
            }
            break;
            
            default:
            {
                ES_DeferEvent(DeferralQueue, ThisEvent);
            }
            break;
        }
        
    }
    break;
    
    
    default:
    break;
  }
  
  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/

static void ConfigureLeaderSPI(void)
{
    uint32_t rData;
    
    //Disable all interrupts
    IEC1CLR = _IEC1_SPI1EIE_MASK;
    IEC1CLR = _IEC1_SPI1RXIE_MASK;
    IEC1CLR = _IEC1_SPI1TXIE_MASK;
            
    SPI1CON = 0;                                // Stop and reset SPI 1
    
    /**********  PIN ASSIGNMENTS (pg. 134-136 in datasheet)  **********/
    //SCK1 automatically set to RB14
//    SS1R = 0b0011;                              // Set SS1 to RB15 // not being used
//    SDI1R = XXXXXX;                             // Not being used
    RPB11R = 0b0011; // Maps SDO1 to RB13                           
//    SDI1R = 0b0011; // Set SDI1 to RB11 (MOSI))
    
    rData = SPI1BUF;                            //Clear buffer
    
    // Clear any existing flags
    IFS1CLR = _IFS1_SPI1EIF_MASK;
    IFS1CLR = _IFS1_SPI1RXIF_MASK;
    IFS1CLR = _IFS1_SPI1TXIF_MASK;
    
    IPC7bits.SPI1IP = 7;                        //Set interrupt priority level
//    IPC7bits.SPI1IS = 3;                        //Set interrupt subpriority level
    
    // Re-enable interrupt(s)
//    IEC1SET = _IEC1_SPI1EIE_MASK;
//    IEC1SET = _IEC1_SPI1RXIE_MASK;
//    IEC1SET = _IEC1_SPI1TXIE_MASK;              // Re-enable TX interrupt
    
    SPI1STATCLR = _SPI1STAT_SPIROV_MASK;        // Clear the Overflow
    
    SPI1BRG = 10;                               // Set baud rate  
    
    //SPI 1 Control Register Bits
    SPI1CONCLR = _SPI1CON_FRMEN_MASK;           // Disable Framed SPI Mode
    SPI1CONSET = _SPI1CON_ENHBUF_MASK;          // Enable Enhanced Buffer Mode
    SPI1CONCLR = _SPI1CON_FRMPOL_MASK;          // SS line is active-low
    SPI1CONCLR = _SPI1CON_MCLKSEL_MASK;         // PBCLK used for baud rate
    SPI1CONCLR = _SPI1CON_MODE32_MASK;          // Put in 8-bit mode (1 of 2)
    SPI1CONCLR = _SPI1CON_MODE16_MASK;          // Put in 8-bit mode (2 of 2)
    SPI1CONCLR = _SPI1CON_SMP_MASK;             // Data sampled at middle
    SPI1CONCLR = _SPI1CON_CKE_MASK;             // On active-to-idle transition
    SPI1CONSET = _SPI1CON_CKP_MASK;             // Idle = High , Active = Low
    SPI1CONSET = _SPI1CON_MSTEN_MASK;           // Enable Master Mode
    SPI1CONSET = _SPI1CON_DISSDI_MASK;          // Disable SDI (Send-Only)
    SPI1CONCLR = _SPI1CON_MSSEN_MASK;           // Disable SS line (controlled by user)
    
    SPI1CONbits.STXISEL = 0b00;     // TX int. thrown when last shifted out
    //SPI1CONbits.SRXISEL = 0b01;     // RX int. thrown when not empty
    
    SPI1CONSET = _SPI1CON_ON_MASK;              //Enable SPI1
}


void __ISR(_SPI_1_VECTOR,IPL7SOFT)__SPI1_ISR(void)
{
    //printf("\n\rEntering SPI ISR...\r\n");
    
    /*if (IFS1bits.SPI1RXIF)      // If receive buffer full intrpt flag
    {
        //printf("\n\n\r<<Receive buffer full interrupt>>\n\r");
        
        static uint16_t ThisByte;
        
        do{
            ThisByte = (uint16_t)SPI1BUF;
            //printf("ThisByte: %x\n\r", ThisByte);

            if (ThisByte != LastByte)
            {
//                printf("New byte received: %x\n\r", ThisByte);
                
                if (0xFF == ThisByte)
                {
                    printf("\n<<< NEW COMMAND INCOMING... >>>\n\n\r");
                }
                else
                {
                    ES_Event_t NewEvent;
                    NewEvent.EventType = NEW_CMD;
                    NewEvent.EventParam = ThisByte;
                    PostSPIService(NewEvent);
                }
                //LastByte = ThisByte;
            }
            else
            {
//                printf("New command not ready. LastByte: %x\n\r",LastByte);
            }
            
            LastByte = ThisByte;
        }while(!SPI1STATbits.SPIRBE);
        
        
    }*/
    
    if (IFS1bits.SPI1TXIF) //If transmit buffer empty intrpt flag
    {
//        printf("\n\n\r<<Transmit buffer empty interrupt>>\n\r");

        if ( (1 == SPI1STATbits.SRMT) && (1 == SPI1STATbits.SPITBE) )
        {
            // Set all CS high
//            if (0 == PORTBbits.RB12)
//            {
//                LATBbits.LATB12 = 1; // CS drivetrain
//            }
//            
//            if (0 == PORTBbits.RB15)
//            {
//                LATBbits.LATB15 = 1; // CS launcher
//            }
            LATBSET = BIT12HI | BIT15HI;
            
            CurrentState = Waiting;
            ES_RecallEvents(MyPriority, DeferralQueue);
            
            IEC1bits.SPI1TXIE = 0;                  // Disable transmit intrpt
        }
        
//        ES_Event_t NewEvent;
//        NewEvent.EventType = SPI_XFER;
//        NewEvent.EventParam = QUERY_BYTE;
//        PostSPIService(NewEvent);
    }
    
    //Clear all flags
    IFS1bits.SPI1EIF = 0;
    IFS1bits.SPI1RXIF = 0;
    IFS1bits.SPI1TXIF = 0;
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

