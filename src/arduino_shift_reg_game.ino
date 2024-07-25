#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MemoryFree.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define MAX_TEXT_LENGTH 40 // Buffer size for outputs
#define GAME_COUNT 2


/*
 *  Input Pins
 */
// *Note* confirm and exit button are connected to pin 2 since only pins 2 and 3 have ISR.
uint8_t confirmButton = 2; // Atmega pin 4
uint8_t exitButton = 3;    // atmega pin 5
uint8_t rightButton = 4;   // atmega pin 6
uint8_t leftButton = 5;    // atmega pin 11


/*
 *  Output Pins
 */

 // LEDS
// Exit button red LED
uint8_t exitLedPin = 10;
// Left button yellow LED
uint8_t leftLedPin = 11;
// Confirm button green LED
uint8_t confirmLedPin = 12;
// Right button blue LED
uint8_t rightLedPin = 13;

// Shift register
// Pin connected to DS of 74HC595
uint8_t dataPin = 7; // Pin 13 on ATMEGA
// Pin connected to SH_CP of 74HC595
uint8_t clockPin = 6; // Atmega pin 12
// Pin connected to ST_CP of 74HC595
uint8_t latchPin = 9; // Atmega pin 15

// Setup for display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Mode selected by user. 0 is default, increase by 1 to the right and wrap around when going left
uint8_t modeSelect = 0;
char modeName[MAX_TEXT_LENGTH];
char gameModeNames[GAME_COUNT][16] = {"Count 256", "Stoplight"};
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
void ledControl(uint8_t number)
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
void setOled(const char* output, uint8_t textSize, uint8_t xLoc=0, uint8_t yLoc=10)
{
  display.clearDisplay();
  display.setCursor(xLoc, yLoc);
  display.setTextSize(textSize);
  display.setTextColor(WHITE);
  display.println(output);
  display.display();
  delay(200);
}

// Called when the user wants to exit the game
void exitToMenu ()
{
  exitInterrupt = false;
  setOled("Returning to main menu...", 2);
  delay(500);
}

void waitForButtonPress(uint8_t buttonPin)
{
  while (digitalRead(buttonPin) == HIGH) {}
  while (digitalRead(buttonPin) == LOW) {}
}

// Activate the chosen mode for the user
void activateMode(uint8_t modeSelect)
{
  uint8_t numberToDisplay = 0;
  char toDisplay[3];
  switch(modeSelect)
  {
    // Count 256
    case 0:
      while (1) 
      {
        if (exitInterrupt == true)
        {
          exitToMenu();
          return;
        }

        ledControl(numberToDisplay);
        sprintf(toDisplay, "%d", numberToDisplay);
        setOled(toDisplay, 3);
        // pause before next value:
        ++numberToDisplay;
      }
      break;
    
    // Stoplight Game
    case 1:
      uint8_t targetValue = 1;
      // Wait for them to hit it, then wait for it to be released
      setOled("Hit confirm on the target light to score!\n\n        ->", 1);
      waitForButtonPress(rightButton);
      confirmInterrupt = false;
      exitInterrupt = false;
      while (true)
      {
        setOled("Set?(confirm)\n\n     <- Other value ->", 2);

        if (digitalRead(confirmButton) == LOW)
        {
           // Wait for single press to finish
           while(digitalRead(confirmButton) == LOW) {}
           break;
        }

        else if (digitalRead(exitButton) == LOW)
        {
          exitToMenu();
          return;
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

      // Forever-loop contingent on whether or not they exit
      while (true)
      { 
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
            delay(1000);
            levelWon = true;
          }
          confirmInterrupt = false;
        }

        // They quit
        if (exitInterrupt == true)
        {
          exitInterrupt = false;
          setOled("Returning to main menu...", 2);
          delay(500);
          return;
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
              setOled("Next level, good luck!", 2);
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
              exitToMenu();
              return;
            }

            else
            {
              setOled("Next level?\n Confirm to continue, or exit to stop", 1);
            }
          }
        }

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

  // LEDs
  pinMode(exitLedPin, OUTPUT);
  pinMode(confirmLedPin, OUTPUT);
  pinMode(leftLedPin, OUTPUT);
  pinMode(rightLedPin, OUTPUT);

  // Shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  // Attach interrupts to buttons
  // Confirm on pin 2
  attachInterrupt(digitalPinToInterrupt(confirmButton), handleConfirmInterrupt, CHANGE);
  // Exit on pin 3
  attachInterrupt(digitalPinToInterrupt(exitButton), handleExitInterrupt, CHANGE);

  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  Serial.println("setup complete");
  Serial.print("freeMemory()=");
  Serial.println(freeMemory());
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
    const char* modeName = gameModeNames[modeSelect];
    // Menu Message
    snprintf(buf, sizeof(buf), "    %s\n\n<- L  Confirm  R ->\n\n%s", modeName, toDisplay);
    setOled(buf, 1);
    ledControl(modeSelect);
  }  
}
