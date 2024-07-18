

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AlmostRandom.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


/*
 *  Input Pins
 */
// *Note* confirm and exit button are connected to pin 2 since only pins 2 and 3 have ISR.
int confirmButton = 2;
int exitButton = 3;
int rightSelectButton = 4;
int leftSelectButton = 5;


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
String modeName;
String gameModeNames[2] = {"Count 256", "Stoplight"};
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
void setOled(String output, int textSize, int xLoc=0, int yLoc=10)
{
  display.clearDisplay();
  display.setCursor(xLoc, yLoc);
  display.setTextSize(textSize);
  display.println(output);
  display.display();
}

// Get the name of the game from the array
String getGameModeName(uint8_t gameModeNumber)
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
        setOled(String(numberToDisplay), 3);
        // pause before next value:
        delay(500);
      }
      break;
    
    // Stoplight Game
    case 1:
      setOled("Hit confirm on the blinking LED to score!", 1);
      uint8_t value = 1;
      uint8_t targetValue = 128;
      while (true)
      {
        // Shift over a bit
        value = value << 1;
        ledControl(value);
        if (confirmInterrupt == true)
        {
          if (value == targetValue)
          {
            setOled("Nice! You win!", 2);
            // Audio chime
            delay(2000);
            confirmInterrupt = false;
            break;
          }
          confirmInterrupt = false;
        }
        delay(500);
      }
      setOled("Next level? (confirm to confirm!)", 1);
      // TODO loop and check before leaving
      delay(2000);
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
  pinMode(rightSelectButton, INPUT_PULLUP);
  pinMode(leftSelectButton, INPUT_PULLUP);
  
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
  display.setTextColor(WHITE);
  delay(1000);
}

void loop() 
{  
  // Wait for a button to be pressed (pullup active, always high unless connected to ground
  if (digitalRead(confirmButton) == LOW)
  {
    activateMode(modeSelect);
  }

  else if (digitalRead(rightSelectButton) == LOW)
  {
    // Wait for button to finish being pressed
    while (digitalRead(rightSelectButton) == LOW) {}
    // Gets the number of objects in the array (each string is 6 bytes, so 3*6 = 18 bytes, 18bytes/6bytes per entry = 3 entries) 
    if (modeSelect < (sizeof(gameModeNames)/sizeof(gameModeNames[0])-1))
    {
      ++modeSelect;
    }
  }

  else if (digitalRead(leftSelectButton) == LOW)
  {
    // Wait for button to finish being pressed
    while (digitalRead(leftSelectButton) == LOW) {}
    // Don't wrap around for now
    if (modeSelect != 0)
    {
      --modeSelect;
    }
  }

  else
  {
    String modeNumber;
    modeNumber = modeSelect;
    modeName = getGameModeName(modeSelect);
    // Menu Message
    setOled("    " + modeName + "\n\n<- L  Confirm  R ->\n\n" + modeNumber, 1);
    ledControl(modeSelect);
  }  
}
