#include <serialize.h>
#include <stdarg.h>
#include <math.h>
#include <Servo.h>

#include "packet.h"
#include "constants.h"

// Number of ticks per revolution from the wheel encoder.

#define COUNTS_PER_REV      4

// Wheel circumference in cm.
// We will use this to calculate forward/backward distance traveled 
// by taking revs * WHEEL_CIRC

#define WHEEL_CIRC          20

#define ALEX_LENGTH         25

#define ALEX_BREADTH        15

#define PIE                 3.141592654

Servo servoLeft;
Servo servoRight;
Servo servoMed;

int leftOpen =150;
int leftClose = 90;
int rightOpen = 30;
int rightClose = 90;
int medClose = 90;
int medOpen = 180;

int S0 = 40;
int S1 = 42;
int S2 = 31;
int S3 = 29;
int sensorOut = 33;

unsigned long redFreq;
unsigned long greenFreq;
unsigned long blueFreq;
unsigned long clear;

unsigned long deltaTicks;
unsigned long targetTicks;

float alexDiagonal = 0.0;
float alexCirc = 0.0;
unsigned long computeDeltaTicks(float ang)
{
  unsigned long ticks = (unsigned long) ((ang * alexCirc * COUNTS_PER_REV) / (360.0 * WHEEL_CIRC));

  return ticks;
}



volatile TDirection dir;

/*
 * Alex's configuration constants
 */


/*
 *    Alex's State Variables
 */


// Store the ticks from Alex's left and right encoders.
volatile unsigned long leftForwardTicks; 
volatile unsigned long rightForwardTicks;
volatile unsigned long leftReverseTicks; 
volatile unsigned long rightReverseTicks;

// Left and right encoder ticks for turning
volatile unsigned long leftForwardTicksTurns; 
volatile unsigned long rightForwardTicksTurns;
volatile unsigned long leftReverseTicksTurns; 
volatile unsigned long rightReverseTicksTurns;

// Store the revolutions on Alex's left and right wheels
volatile unsigned long leftRevs;
volatile unsigned long rightRevs;

// Forward and backward distance traveled
volatile unsigned long forwardDist;
volatile unsigned long reverseDist;


//for distance 
unsigned long deltaDist;
unsigned long newDist;


void left(float ang, float speed) {
  if(ang==0){
    deltaTicks=99999999;
  }
  else{
    deltaTicks=computeDeltaTicks(ang); 
  }
  targetTicks = leftReverseTicksTurns + deltaTicks;
  ccw(ang,speed);
}

void right(float ang, float speed) {
  if(ang==0){
    deltaTicks=99999999;
  }
  else{
    deltaTicks=computeDeltaTicks(ang); 
  }
  targetTicks = rightReverseTicksTurns + deltaTicks;
  cw(ang, speed);
}




/*
 * 
 * Alex Communication Routines.
 * 
 */
 
TResult readPacket(TPacket *packet)
{
    // Reads in data from the serial port and
    // deserializes it.Returns deserialized
    // data in "packet".
    
    char buffer[PACKET_SIZE];
    int len;

    len = readSerial(buffer);

    if(len == 0)
      return PACKET_INCOMPLETE;
    else
      return deserialize(buffer, len, packet);
    
}

void sendStatus()
{
  // Implement code to send back a packet containing key
  // information like leftTicks, rightTicks, leftRevs, rightRevs
  // forwardDist and reverseDist
  // Use the params array to store this information, and set the
  // packetType and command files accordingly, then use sendResponse
  // to send out the packet. See sendMessage on how to use sendResponse.
  //

  TPacket statusPacket;
  statusPacket.packetType=PACKET_TYPE_RESPONSE;
  statusPacket.command=RESP_STATUS;
  statusPacket.params[0] = leftForwardTicks;
  statusPacket.params[1] = rightForwardTicks;
  statusPacket.params[2] = leftReverseTicks;
  statusPacket.params[3] = rightReverseTicks;
  statusPacket.params[4] = leftForwardTicksTurns;
  statusPacket.params[5] = rightForwardTicksTurns;
  statusPacket.params[6] = leftReverseTicksTurns;
  statusPacket.params[7] = rightReverseTicksTurns;
  statusPacket.params[8] = forwardDist;
  statusPacket.params[9] = reverseDist;
  sendResponse(&statusPacket);

}

void sendMessage(const char *message)
{
  // Sends text messages back to the Pi. Useful
  // for debugging.
  
  TPacket messagePacket;
  messagePacket.packetType=PACKET_TYPE_MESSAGE;
  strncpy(messagePacket.data, message, MAX_STR_LEN);
  sendResponse(&messagePacket);
}

void dbprintf(const char *format, ...) {
  va_list args;
  char buffer[128];

  va_start(args, format);
  vsprintf(buffer, format, args);
  sendMessage(buffer);
}  

void sendBadPacket()
{
  // Tell the Pi that it sent us a packet with a bad
  // magic number.
  
  TPacket badPacket;
  badPacket.packetType = PACKET_TYPE_ERROR;
  badPacket.command = RESP_BAD_PACKET;
  sendResponse(&badPacket);
  
}

void sendBadChecksum()
{
  // Tell the Pi that it sent us a packet with a bad
  // checksum.
  
  TPacket badChecksum;
  badChecksum.packetType = PACKET_TYPE_ERROR;
  badChecksum.command = RESP_BAD_CHECKSUM;
  sendResponse(&badChecksum);  
}

void sendBadCommand()
{
  // Tell the Pi that we don't understand its
  // command sent to us.
  
  TPacket badCommand;
  badCommand.packetType=PACKET_TYPE_ERROR;
  badCommand.command=RESP_BAD_COMMAND;
  sendResponse(&badCommand);
}

void sendBadResponse()
{
  TPacket badResponse;
  badResponse.packetType = PACKET_TYPE_ERROR;
  badResponse.command = RESP_BAD_RESPONSE;
  sendResponse(&badResponse);
}



void sendOK()
{
  TPacket okPacket;
  okPacket.packetType = PACKET_TYPE_RESPONSE;
  okPacket.command = RESP_OK;
  sendResponse(&okPacket);  
}



void sendResponse(TPacket *packet)
{
  // Takes a packet, serializes it then sends it out
  // over the serial port.
  char buffer[PACKET_SIZE];
  int len;

  len = serialize(buffer, packet, sizeof(TPacket));
  writeSerial(buffer, len);
}


/*
 * Setup and start codes for external interrupts and 
 * pullup resistors.
 * 
 */
// Enable pull up resistors on pins 18 and 19
void enablePullups()
{
  // Use bare-metal to enable the pull-up resistors on pins
  // 19 and 18. These are pins PD2 and PD3 respectively.
  // We set bits 2 and 3 in DDRD to 0 to make them inputs. 
  DDRD &= 0b11110011;
  PORTD |= 0b00001100;
}

// Functions to be called by INT2 and INT3 ISRs.
void leftISR()
{
  if (dir == FORWARD) {
    leftForwardTicks++;
    forwardDist = (unsigned long)((float)leftForwardTicks / COUNTS_PER_REV * WHEEL_CIRC);
  } else if (dir == BACKWARD) {
    leftReverseTicks++;
    reverseDist = (unsigned long)((float)leftReverseTicks / COUNTS_PER_REV * WHEEL_CIRC);
  } else if (dir == LEFT) {
    leftReverseTicksTurns++;
  } else if (dir == RIGHT) {
    leftForwardTicksTurns++;
  }
}

void rightISR()
{
  if (dir == FORWARD) {
    rightForwardTicks++;
  } else if (dir == BACKWARD) {
    rightReverseTicks++;
  } else if (dir == LEFT) {
    rightForwardTicksTurns++;
  } else if (dir == RIGHT) {
    rightReverseTicksTurns++;
  }
}

// Set up the external interrupt pins INT2 and INT3
// for falling edge triggered. Use bare-metal.
void setupEINT()
{
  // Use bare-metal to configure pins 18 and 19 to be
  // falling edge triggered. Remember to enable
  // the INT2 and INT3 interrupts.
  // Hint: Check pages 110 and 111 in the ATmega2560 Datasheet.
  EICRA = 0b10100000;
  EIMSK = 0b00001100;
}

// Implement the external interrupt ISRs below.
// INT3 ISR should call leftISR while INT2 ISR
// should call rightISR.
ISR(INT2_vect)
{
  rightISR();
}

ISR(INT3_vect)
{
  leftISR();
}
// Implement INT2 and INT3 ISRs above.

/*
 * Setup and start codes for serial communications
 * 
 */
// Set up the serial connection. For now we are using 
// Arduino Wiring, you will replace this later
// with bare-metal code.

void setupColour()
{
  pinMode(S0,OUTPUT);
  pinMode(S1,OUTPUT);
  pinMode(S2,OUTPUT);
  pinMode(S3,OUTPUT);
  pinMode(sensorOut,INPUT_PULLUP);
}

void setupSerial()
{
  // To replace later with bare-metal.
  Serial.begin(9600);
  // Change Serial to Serial2/Serial3/Serial4 in later labs when using the other UARTs
}

// Start the serial connection. For now we are using
// Arduino wiring and this function is empty. We will
// replace this later with bare-metal code.

void startSerial()
{
  // Empty for now. To be replaced with bare-metal code
  // later on.
  
}

// Read the serial port. Returns the read character in
// ch if available. Also returns TRUE if ch is valid. 
// This will be replaced later with bare-metal code.

int readSerial(char *buffer)
{

  int count=0;

  // Change Serial to Serial2/Serial3/Serial4 in later labs when using other UARTs

  while(Serial.available())
    buffer[count++] = Serial.read();

  return count;
}

// Write to the serial port. Replaced later with
// bare-metal code

void writeSerial(const char *buffer, int len)
{
  Serial.write(buffer, len);
  // Change Serial to Serial2/Serial3/Serial4 in later labs when using other UARTs
}

/*
 * Alex's setup and run codes
 * 
 */

// Clears all our counters
void clearCounters()
{
// Store the ticks from Alex's left and right encoders.
  leftForwardTicks = 0;
  rightForwardTicks = 0;
  leftReverseTicks = 0;
  rightReverseTicks = 0;

// Left and right encoder ticks for turning
  leftForwardTicksTurns = 0; 
  rightForwardTicksTurns = 0;
  leftReverseTicksTurns = 0; 
  rightReverseTicksTurns = 0;

// Store the revolutions on Alex's left and right wheels
  leftRevs = 0;
  rightRevs = 0;

// Forward and backward distance traveled
  forwardDist = 0;
  reverseDist = 0;
}

// Clears one particular counter
void clearOneCounter(int which)
{
  clearCounters();
}

// Intialize Alex's internal states
void initializeState()
{
  clearCounters();
}

void handleCommand(TPacket *command)
{
  switch(command->command)
  {
    // For movement commands, param[0] = distance, param[1] = speed.
    case COMMAND_FORWARD:
        sendOK();
        forward((double) command->params[0], (float) command->params[1]);
      break;

    case COMMAND_REVERSE:
        sendOK();
        backward((double) command->params[0], (float) command->params[1]);
      break;

    case COMMAND_TURN_LEFT:
        sendOK();
        left((double) command->params[0], (float) command->params[1]);
      break;

    case COMMAND_TURN_RIGHT:
        sendOK();
        right((double) command->params[0], (float) command->params[1]);
      break;

    case COMMAND_STOP:
        sendOK();
        stop();
      break;

    /*
     * Implement code for other commands here.
     * 
     */
     case COMMAND_OPEN:
          sendOK();
          clawOpen();
          break;

     case COMMAND_CLOSE:
          sendOK();
          clawClose();
          break;

     case COMMAND_DROP:
          sendOK();
          dropMed();
          break;


     case COMMAND_SCAN:
         sendOK();
         scanColor();
         sendColor();
         
         break;
     
     case COMMAND_GET_STATS:
         sendStatus();
         break;

     case COMMAND_CLEAR_STATS:
         sendOK();
         clearOneCounter(command->params[0]);
         break;
     
        
    default:
      sendBadCommand();
  }
}

void waitForHello()
{
  int exit=0;

  while(!exit)
  {
    TPacket hello;
    TResult result;
    
    do
    {
      result = readPacket(&hello);
    } while (result == PACKET_INCOMPLETE);

    if(result == PACKET_OK)
    {
      if(hello.packetType == PACKET_TYPE_HELLO)
      {
     

        sendOK();
        exit=1;
      }
      else
        sendBadResponse();
    }
    else
      if(result == PACKET_BAD)
      {
        sendBadPacket();
      }
      else
        if(result == PACKET_CHECKSUM_BAD)
          sendBadChecksum();
  } // !exit
}

void setup() {
  // put your setup code here, to run once:
  alexDiagonal = sqrt((ALEX_LENGTH * ALEX_LENGTH) + (ALEX_BREADTH * ALEX_BREADTH));

  alexCirc = PIE * alexDiagonal;
  servoLeft.attach(10);
  servoRight.attach(9);
  servoMed.attach(22);
  servoLeft.write(leftOpen);
  servoRight.write(rightOpen);
  servoMed.write(medClose);


  setupColour();
  
  cli();
  setupEINT();
  setupSerial();
  startSerial();
  enablePullups();
  initializeState();
  sei();

}

void handlePacket(TPacket *packet)
{
  switch(packet->packetType)
  {
    case PACKET_TYPE_COMMAND:
      handleCommand(packet);
      break;

    case PACKET_TYPE_RESPONSE:
      break;

    case PACKET_TYPE_ERROR:
      break;

    case PACKET_TYPE_MESSAGE:
      break;

    case PACKET_TYPE_HELLO:
      break;
  }
}

void loop() {
// Uncomment the code below for Step 2 of Activity 3 in Week 8 Studio 2
 // forward(0, 100);


  //printLiveColor(); //print rgb values in serial monitor
  //delay(700);

 // put your main code here, to run repeatedly:
  TPacket recvPacket; // This holds commands from the Pi

  TResult result = readPacket(&recvPacket);
  
  if(result == PACKET_OK)
    handlePacket(&recvPacket);
  else
    if(result == PACKET_BAD)
    {
      sendBadPacket();
    }
    else
      if(result == PACKET_CHECKSUM_BAD)
      {
        sendBadChecksum();
      } 
      
  if(deltaDist > 0)
  {
   if(dir==FORWARD)
   {
    if(forwardDist > newDist)
    {
     deltaDist = 0;
     newDist = 0;
     stop();    
    }
   }
   else
    if(dir==BACKWARD)
    {
      if(reverseDist>newDist)
      {
        deltaDist = 0;
        newDist = 0;
        stop();
      }
    }
    else 
    
  if(dir == (TDirection) STOP)
       {
        deltaDist = 0;
        newDist = 0;
        stop();
       }
  }

if(deltaTicks > 0)
  {
   if(dir==LEFT)
   {
    if(leftReverseTicksTurns >= targetTicks)
    {
     deltaTicks = 0;
     targetTicks = 0;
     stop();    
    }
   }
   else
    if(dir==RIGHT)
    {
      if(rightReverseTicksTurns >= targetTicks)
    {
     deltaTicks = 0;
     targetTicks = 0;
     stop();    
    }
   }
    else 
    
  if(dir == (TDirection) STOP)
       {
      deltaTicks = 0;
      targetTicks = 0;
      stop();
       }
  }
}
