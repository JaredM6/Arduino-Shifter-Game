

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AlmostRandom.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define MAX_TEXT_LENGTH 64 // Buffer size for outputs
#define GAME_COUNT 2


/*
 *  Input Pins
 */
// *Note* confirm and exit button are connected to pin 2 since only pins 2 and 3 have ISR.
int confirmButton = 2;
int exitButton = 3;
int rightButton = 4;
int leftButton = 5;


/*
 *  Output Pins
 */
// Pin connected to DS of 74HC595
int dataPin = 7;
// Pin connected to SH_CP of 74HC595
int clockPin = 6;
// Pin connected to ST_CP of 74HC595
int latchPin = 8;

// Setup for display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Mode selected by user. 0 is default, increase by 1 to the right and wrap around when going left
uint8_t modeSelect = 0;
char modeName[MAX_TEXT_LENGTH];
char gameModeNames[GAME_COUNT][MAX_TEXT_LENGTH] = {"Count 256", "Stoplight"};
volatile bool confirmInterrupt = false;
volatile bool exitInterrupt = false;


/*
 * ISR Functions
 */
// ISR for pin 2
void handleConfirmInterrupt()
{
  confirmInterrupt = true;
}

// ISR for pin 3
void handleExitInterrupt()
{
  exitInterrupt = true;
}


/*
 * Custom Functions
 */
// Send the data to the shifter
void ledControl(int number)
{
  // take the latchPin low so the LEDs don't change while you're sending in bits
  digitalWrite(latchPin, LOW);
  // shift out the bits
  shiftOut(dataPin, clockPin, MSBFIRST, number);
  //take the latch pin high so the LEDs will light up
  digitalWrite(latchPin, HIGH);
}

// Set the options for the display
// *NOTE* text size ranges from 1-9
void setOled(const char* output, int textSize, int xLoc=0, int yLoc=10)
{
  display.clearDisplay();
  display.setCursor(xLoc, yLoc);
  display.setTextSize(textSize);
  display.setTextColor(WHITE);
  display.println(output);
  display.display();
  delay(200);
}

void waitForButtonPress(int buttonPin)
{
  while (digitalRead(buttonPin) == HIGH) {}
  while (digitalRead(buttonPin) == LOW) {}
}

// Get the name of the game from the array
char* getGameModeName(uint8_t gameModeNumber)
{
  return gameModeNames[gameModeNumber];
}

// Activate the chosen mode for the user
void activateMode(uint8_t modeSelect)
{
  switch(modeSelect)
  {
    // Count 256
    case 0:
      for (int numberToDisplay = 0; numberToDisplay < 256; numberToDisplay++) 
      {
        if (exitInterrupt == true)
        {
          exitInterrupt = false;
          break;
        }
        ledControl(numberToDisplay);
        char toDisplay[3];
        sprintf(toDisplay, "%d", numberToDisplay);
        setOled(toDisplay, 3);
        // pause before next value:
        delay(500);
      }
      break;
    
    // Stoplight Game
    case 1:
      uint8_t targetValue = 1;
      // Wait for them to hit it, then wait for it to be released
      setOled("Test!", 1);
      waitForButtonPress(rightButton);
      confirmInterrupt = false;
      exitInterrupt = false;
      bool done = false;
      while (true)
      {
        setOled("Set?(confirm)\n\n   Other value ->", 1);
        if (digitalRead(confirmButton) == LOW)
        {
           // Wait for single press to finish
           while(digitalRead(confirmButton) == LOW) {}
           break;
        }

        else if ((digitalRead(rightButton) == LOW) and (targetValue != 128))
        {
          while(digitalRead(rightButton) == LOW) {} 
          targetValue = targetValue << 1;
        }

        else if ((digitalRead(leftButton) == LOW) and (targetValue != 1))
        {
          while(digitalRead(leftButton) == LOW) {}
          targetValue = targetValue / 2;
        }

        else
        {
          ledControl(targetValue);
        }
      }
      uint8_t value = 1;
      int delayValue = 1000;
      bool levelWon = false;
      bool wantToExit = false;

      // Forever-loop contingent on whether or not they exit
      while (wantToExit == false)
      {
        // Wrap around if on last LED
        if (value == 128)
        {
          value = 1; 
        }

        else
        {
          // Shift over a bit
          value = value << 1;
        }
        
        ledControl(value);

        // Clear the interrupt flag, since the only window to click is the delay window. There is a
        // scenario where they click it, set to true, and the value then changes to target (easy win).
        confirmInterrupt = false;
        // Pause length is the difficulty
        delay(delayValue);
        
        // Did they hit the button at the correct value?
        if (confirmInterrupt == true)
        {
          if (value == targetValue)
          {
            setOled("Nice! You win!", 2);
            // Audio chime
            delay(2000);
            levelWon = true;
          }
          confirmInterrupt = false;
        }

        // They quit
        if (exitInterrupt == true)
        {
          exitInterrupt = false;
          setOled("Returning to main menu", 2);
          wantToExit = true;
        }

        // Did they win? Time to check for next level or exit
        if (levelWon == true)
        {
          // Set to false in case exit was tripped on accident
          exitInterrupt = false;
          confirmInterrupt = false;

          // Only 2 options
          while (true)
          {
            if (confirmInterrupt == true)
            {
              confirmInterrupt = false;
              delay(2000);
              delayValue = delayValue / 2;
              levelWon = false;
              // Clever way to wrap back to 1 right at the start
              value = 128;
              break;
            }
            else if (exitInterrupt == true)            
            {
              exitInterrupt = false;
              wantToExit = true;
              break;
            }
            else
            {
              setOled("Next level?", 1);
            }
          }
        }
      }
      break;
  }
}


/*
 * Arduino Functions (setup and loop)
 */
void setup() 
{
  // Inputs
  pinMode(confirmButton, INPUT_PULLUP);
  pinMode(exitButton, INPUT_PULLUP);
  pinMode(rightButton, INPUT_PULLUP);
  pinMode(leftButton, INPUT_PULLUP);
  
  // Outputs
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  // Attach interrupts to buttons
  // Confirm on pin 2
  attachInterrupt(digitalPinToInterrupt(confirmButton), handleConfirmInterrupt, CHANGE);
  // Exit on pin 3
  attachInterrupt(digitalPinToInterrupt(exitButton), handleExitInterrupt, CHANGE);

  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(1000);
}

void loop() 
{  
  // Wait for a button to be pressed (pullup active, always high unless connected to ground
  if (digitalRead(confirmButton) == LOW)
  {
    activateMode(modeSelect);
  }

  else if (digitalRead(rightButton) == LOW)
  {
    // Wait for button to finish being pressed
    while (digitalRead(rightButton) == LOW) {}
    // Gets the number of objects in the array (each string is 6 bytes, so 3*6 = 18 bytes, 18bytes/6bytes per entry = 3 entries) 
    if (modeSelect < (sizeof(gameModeNames)/sizeof(gameModeNames[0])-1))
    {
      ++modeSelect;
    }
  }

  else if (digitalRead(leftButton) == LOW)
  {
    // Wait for button to finish being pressed
    while (digitalRead(leftButton) == LOW) {}
    // Don't wrap around for now
    if (modeSelect != 0)
    {
      --modeSelect;
    }
  }

  else
  {
    // Get a string of the number
    char toDisplay[3];
    sprintf(toDisplay, "%d", modeSelect);
    // Get the name from the index
    char buf[MAX_TEXT_LENGTH];
    const char* modeName = getGameModeName(modeSelect);
    // Menu Message
    snprintf(buf, sizeof(buf), "    %s\n\n<- L  Confirm  R ->\n\n%s", modeName, toDisplay);
    setOled(buf, 1);
    ledControl(modeSelect);
  }  
}
