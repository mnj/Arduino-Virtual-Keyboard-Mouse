#include <Keyboard.h>
#include <AbsMouse.h>

/*
  This code relies on sending commands over the serial interface, which will 
  then perform various actions with the mouse and keyboard

  The data format for the serial commands are a series of bytes, each command 
  should always consist of the start and end marker
  
  Data[0] = 0xfe (start marker)
  Data[1] = One byte, which defines which command it is
  Data[N] = One or more bytes as argument, as required for each command
  Data[N+1] = 0xff (end marker)

  You can't use the start/end marker for any of the command/command argument bytes
  - Will be fixed at a later time with a finite state machine instead

  We could just use JSON or simpler custom text based packets, but some of the use cases
  for this needs to be really fast and efficient, so using bytes for commands/args for now.
*/

// Receiving serial data variables
const byte numBytes = 16;
byte receivedBytes[numBytes];
byte numReceived = 0;
bool newData = false;

bool shouldPressKeyContinuously = false;
char keyToPressContinuously = 49;
int continuousKeyPressDelay = 25;
bool mouseInitialized = false;

/*
  Checks if there is any data available on the serial interface, if there is 
  then uses the start/end markers to find out when all has been received, 
  this will happen over multiple calls to his function

  FIXME: Replace this mess, with a finite state machine instead
*/
void RecvBytesFromSerial()
{
  static boolean recvInProgress = false;
  static byte ndx = 0;
  byte startMarker = 0xfe;
  byte endMarker = 0xff;
  byte rb;

  while (Serial.available() > 0 && newData == false)
  {
    rb = Serial.read();

    if (recvInProgress == true)
    {
      if (rb != endMarker)
      {
        receivedBytes[ndx] = rb;
        ndx++;

        if (ndx >= numBytes)
        {
          ndx = numBytes - 1;
        }
      }
      else
      {
        receivedBytes[ndx] = 0xff;
        recvInProgress = false;
        numReceived = ndx;
        ndx = 0;
        newData = true;
      }
    }
    else if (rb == startMarker)
    {
      recvInProgress = true;
    }
  }
}

/*
  Takes two 8 bit bytes and converts them to a 16bit int
*/
int ConvertTo16BitInt(byte high, byte low)
{
  int parsedInt;
  parsedInt = high;
  parsedInt = parsedInt << 8;
  parsedInt |= low;

  return parsedInt;
}

/* 
  Initializes the absolute position mouse with the screen resolution

  This expect being passed two 16bit ints
  width = receivedBytes[1] + receivedBytes[2]
  height = receivedBytes[3] + receivedBytes[4]
*/
void InitMouse()
{
  int width = ConvertTo16BitInt(receivedBytes[1], receivedBytes[2]);
  int height = ConvertTo16BitInt(receivedBytes[3], receivedBytes[4]);

  AbsMouse.init(width, height);

  mouseInitialized = true;
}

/*
  The mouse movement coordinates are absolute, based on the resolution set by call the mouse init command

  This function/command expect 2, 16bit ints
  x = receivedBytes[1] + receivedBytes[2]
  y = receivedBytes[3] + receivedBytes[4]
*/
void PerformMouseMove()
{
  int x = ConvertTo16BitInt(receivedBytes[1], receivedBytes[2]);
  int y = ConvertTo16BitInt(receivedBytes[3], receivedBytes[4]);

  /*
    // DEBUG:
    Serial.print("We are moving the cursor to: ");
    Serial.print(" X: ");
    Serial.print(x, DEC);
    Serial.print(" Y: ");
    Serial.print(y, DEC);  
    Serial.print("\n");
  */

  AbsMouse.move(x,y);
}

void PerformKeyboardPress()
{
  Keyboard.press(receivedBytes[1]);

  // Add random delay - (0-15ms)
  delay(random(15));

  Keyboard.releaseAll();
}

/*
  Performs a mouse click, on the current location of the mouse

  Expects a sub command byte, which defines the type of mouse click
  and an optional amount of miliseconds to hold down the button, this
  is implemented as a random interval from 0-hold_time ms.

  button = receivedBytes[1], 0x01 = Left, 0x02 = Middle, 0x03 = Right
  hold_time = receivedBytes[2] 0-253 ms (random number up to this, not specific hold time)
*/
void PerformMouseClick()
{
  // Default hold time 10ms
  int hold_time = 10;
  bool random_hold_time = true;
  
  // Check if we have an optional hold time passed, and make sure,
  // we are not at the end marker
  if(receivedBytes[2] != 0xff)
    hold_time = receivedBytes[2];

  switch(receivedBytes[1])
  {
    case 0x01:
      if(mouseInitialized)
      {
        AbsMouse.press(MOUSE_LEFT);

        delay(random(hold_time));

        AbsMouse.release(MOUSE_LEFT);
      }
      break;
    case 0x02:
      if(mouseInitialized)
      {
        AbsMouse.press(MOUSE_MIDDLE);

        delay(random(hold_time));

        AbsMouse.release(MOUSE_MIDDLE);
      }
      break;
    case 0x03:
      if(mouseInitialized)
      {
        AbsMouse.press(MOUSE_RIGHT);

        delay(random(hold_time));

        AbsMouse.release(MOUSE_RIGHT);
      }
      break;
  }
}

/*
  Ping/Pong command to check that we are succesfully passing commands to the arduino

  Does not expect any arguments to be passed
*/
void Ping()
{
  Serial.write("PONG\n");
}

/*
  Process the serial commands that are passed in, as the first byte after the start marker
*/
void ProcessSerialData()
{
  if (newData == true)
  {
    // Do stuff with the data (the byte array is excl the start/end marker)
    switch (receivedBytes[0])
    {
      case 0x01:
        Ping();
        break;
      case 0x03:
        PerformKeyboardPress();
        break;
      case 0x04:
        InitMouse();
        break;
      case 0x05:
        PerformMouseMove();
        break;
      case 0x06:
        PerformMouseClick();
        break;
      case 0x07:
        InitContinuousKeyPress();
        break;
      case 0x08:
        shouldPressKeyContinuously = true; // Used by command 0x07
        break;
      case 0x09:
        shouldPressKeyContinuously = false; // Used by command 0x08
        break;
    }

    newData = false;
  }
}

/*
  Sets up which key should be continously be pressed, if enabled with option 0x08

  Expects a byte with the key to press, and optionally a delay between 0-253 ms
  key = receivedBytes[1] - the key to press
  delay = receivedBytes[2] - optionally delay between key presses, default 25ms
*/
void InitContinuousKeyPress()
{
  int delay = 25;

  // Check if we have an optional delay time passed, and make sure,
  // we are not at the end marker
  if(receivedBytes[2] != 0xff)
    delay = receivedBytes[2];

  keyToPressContinuously = receivedBytes[1];

  /*
  // DEBUG
  Serial.print("We will press: ");
  Serial.print(" int: ");
  Serial.print(keyToPressContinuously, DEC);
  Serial.print(" delay: ");
  Serial.print(delay);
  Serial.print("\n");
  */
}

/*
  Keeps pressing the key defined with the init continuous key press command, 
  even if not actively receiving any command.
*/
void PerformContinuousKeyPress()
{
  if (shouldPressKeyContinuously == true)
  {
      Keyboard.press(keyToPressContinuously);

      // Add random hold time delay - (0-10ms)
      delay(random(10));

      Keyboard.releaseAll();

      // Wait after pressing the key, before pressing it again
      delay(continuousKeyPressDelay);
  }
}

/*
  Send a single key press, with an optional hold time

  Expects a byte with the key to press, and optionally a hold time between 0-253 ms
  key = receivedBytes[1] - the key to press
  hold_time = receivedBytes[2] - optionally hold time between key presses, default 25ms

  FIXME: Make it possible to specify modifiers like SHIFT etc
*/
void PerformKeyPress()
{
  int hold_time = 25;

  // Check if we have an optional delay time passed, and make sure,
  // we are not at the end marker
  if(receivedBytes[2] != 0xff)
    hold_time = receivedBytes[2];

  Keyboard.press(receivedBytes[1]);
  delay(hold_time);
  Keyboard.releaseAll();
}

/*
  Sets up the arduino
*/
void setup()
{
  // Enable the serial interface
  Serial.begin(115200);

  while (!Serial)
  {
    ; // Wait for the serial port to be connected
  }

  Serial.write("Configured");

  // Enable the Keyboard USB interface
  Keyboard.begin();

  // Set the serial timeout to 500ms
  Serial.setTimeout(500);
}

/*
  The main loop, that checks for new commands over the serial interface, and then proceses
  the different commands that were passed in
*/
void loop()
{
  RecvBytesFromSerial();
  ProcessSerialData();

  // Special case, this should keep running as long as shouldPressKey = true,
  // even if not actively sending any commands
  PerformContinuousKeyPress();
}
