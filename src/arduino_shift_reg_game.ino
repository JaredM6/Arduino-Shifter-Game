#include <Wire.h>
#include <avr/pgmspace.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MemoryFree.h>


/*
 * Macros
 */
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define MAX_TEXT_LENGTH 40  // Buffer size for outputs
#define MAX_GAME_NAME_LENGTH 16
#define GAME_COUNT 2


/*
 *  Input Pins
 */
// *Note* confirm and exit button are connected to pin 2 and 3 respectively since only pins 2 and 3 have ISR for Uno Rev 3.
#define CONFIRM_PIN 2  // Atmega pin 4
#define EXIT_PIN 3     // atmega pin 5
#define RIGHT_PIN 4    // atmega pin 6
#define LEFT_PIN 5     // atmega pin 11


/*
 *  Output Pins
 */

// Shift register
// Pin connected to DS of 74HC595
#define DS_DATA_PIN 7  // Pin 13 on ATMEGA
// Pin connected to SH_CP of 74HC595
#define SH_CP_CLOCK_PIN 6  // Atmega pin 12
// Pin connected to ST_CP of 74HC595
#define ST_CP_LATCH_PIN 9  // Atmega pin 15


/*
 * Variables
 */
// Constants for flash storage to save some RAM
const char gameModeNames[GAME_COUNT][MAX_GAME_NAME_LENGTH] PROGMEM = { "Count 256", "Stoplight" };

// Non-Constants
// Setup for display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// Mode selected by user. 0 is default, increase by 1 to the right and wrap around when going left
uint8_t modeSelect = 0;
// Interrupt bools
volatile bool confirmInterrupt = false;
volatile bool exitInterrupt = false;


/*
 * ISR Functions
 */
// ISR for pin 2
void handleConfirmInterrupt() {
  confirmInterrupt = true;
}

// ISR for pin 3
void handleExitInterrupt() {
  exitInterrupt = true;
}


/*
 * Custom Functions
 */
// Send the data to the shifter
void shiftOutputControl(uint8_t number) {
  // take the ST_CP_LATCH_PIN low so the LEDs don't change while you're sending in bits
  digitalWrite(ST_CP_LATCH_PIN, LOW);
  // shift out the bits
  shiftOut(DS_DATA_PIN, SH_CP_CLOCK_PIN, MSBFIRST, number);
  //take the latch pin high so the LEDs will light up
  digitalWrite(ST_CP_LATCH_PIN, HIGH);
}

// Set the options for the display
// *NOTE* text size ranges from 1-9
void setOled(const char* output, uint8_t textSize, uint8_t xLoc = 0, uint8_t yLoc = 10) {
  display.clearDisplay();
  display.setCursor(xLoc, yLoc);
  display.setTextSize(textSize);
  display.setTextColor(WHITE);
  display.println(output);
  display.display();
  delay(200);
}

// Called when the user wants to exit the game
void exitToMenu() {
  exitInterrupt = false;
  setOled("Returning to main menu...", 2);
  delay(500);
}

void waitForButtonPress(uint8_t buttonPin) {
  while (digitalRead(buttonPin) == HIGH) {}
  while (digitalRead(buttonPin) == LOW) {}
}

// Get the name from the index
void setMenuScreen(uint8_t modeNumber) {
  char modeName[MAX_GAME_NAME_LENGTH];
  strncpy_P(modeName, gameModeNames[modeNumber], MAX_GAME_NAME_LENGTH);
  // Menu Message
  char buf[MAX_TEXT_LENGTH];
  snprintf(buf, sizeof(buf), "     %s\n\n<- L  Confirm  R ->\n\n", modeName);
  setOled(buf, 1);
  shiftOutputControl(modeNumber);
}

// Activate the chosen mode for the user
void activateMode(uint8_t modeSelect) {
  uint8_t numberToDisplay = 0;
  char toDisplay[3];
  switch (modeSelect) {
    // Count 256
    case 0:
      while (1) {
        if (exitInterrupt == true) {
          exitToMenu();
          return;
        }

        shiftOutputControl(numberToDisplay);
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
      setOled("Hit confirm on the target light to play!\n\n         ->", 1);
      waitForButtonPress(RIGHT_PIN);
      confirmInterrupt = false;
      exitInterrupt = false;
      setOled("Set?(confirm)\n\n   <- Other value ->", 1);
      while (true) {
        if (digitalRead(CONFIRM_PIN) == LOW) {
          // Wait for single press to finish
          while (digitalRead(CONFIRM_PIN) == LOW) {}
          break;
        }

        else if (digitalRead(EXIT_PIN) == LOW) {
          exitToMenu();
          return;
        }

        // Left to increase the value (move the target led left)
        else if ((digitalRead(LEFT_PIN) == LOW) and (targetValue != 128)) {
          while (digitalRead(LEFT_PIN) == LOW) {}
          targetValue = targetValue << 1;
        }

        // Right to decrease the value (move the led right)
        else if ((digitalRead(RIGHT_PIN) == LOW) and (targetValue != 1)) {
          while (digitalRead(RIGHT_PIN) == LOW) {}
          targetValue = targetValue / 2;
        }

        else {
          shiftOutputControl(targetValue);
        }
      }

      uint8_t value = 1;
      uint16_t delayValue = 1000;
      bool levelWon = false;
      setOled("Start!", 3);
      // Forever-loop contingent on whether or not they exit
      while (true) {
        shiftOutputControl(value);

        // Clear the interrupt flag, since the only window to click is the delay window. There is a
        // scenario where they click it, set to true, and the value then changes to target (easy win).
        confirmInterrupt = false;
        // Pause length is the difficulty
        delay(delayValue);

        // Did they hit the button at the correct value?
        if (confirmInterrupt == true) {
          if (value == targetValue) {
            setOled("Nice! You win!", 2);
            // Audio chime
            delay(1000);
            levelWon = true;
          }
          confirmInterrupt = false;
        }

        // They quit
        if (exitInterrupt == true) {
          exitToMenu();
          return;
        }

        // Did they win? Time to check for next level or exit
        if (levelWon == true) {
          // Set to false in case exit was tripped on accident
          exitInterrupt = false;
          confirmInterrupt = false;

          // Only 2 options
          while (true) {
            if (confirmInterrupt == true) {
              setOled("\n\n   Next level, good luck!", 1);
              confirmInterrupt = false;
              delay(2000);
              delayValue = delayValue / 2;
              levelWon = false;
              // Clever way to wrap back to 1 right at the start
              value = 128;
              break;
            }

            else if (exitInterrupt == true) {
              exitToMenu();
              return;
            }

            else {
              setOled("Next level?\n\nConfirm to continue, or exit to stop", 1);
            }
          }
        }

        // Wrap around if on last LED
        if (value == 128) {
          value = 1;
        }

        else {
          // Shift over a bit
          value = value << 1;
        }
      }
      break;
  }
  return;
}


/*
 * Arduino Functions (setup and loop)
 */
void setup() {
  // Inputs
  pinMode(CONFIRM_PIN, INPUT_PULLUP);
  pinMode(EXIT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);

  // Outputs
  // Shift register
  pinMode(ST_CP_LATCH_PIN, OUTPUT);
  pinMode(SH_CP_CLOCK_PIN, OUTPUT);
  pinMode(DS_DATA_PIN, OUTPUT);

  // Attach interrupts to buttons
  // Confirm on pin 2
  attachInterrupt(digitalPinToInterrupt(CONFIRM_PIN), handleConfirmInterrupt, CHANGE);
  // Exit on pin 3
  attachInterrupt(digitalPinToInterrupt(EXIT_PIN), handleExitInterrupt, CHANGE);

  Serial.begin(9600);

  Serial.print("freeMemory()=");
  Serial.println(freeMemory());

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  // Serial.println("setup complete");
  Serial.print("freeMemory()=");
  Serial.println(freeMemory());
  delay(1000);

  setMenuScreen(0);
}

void loop() {
  // Wait for a button to be pressed (pullup active, always high unless connected to ground
  if (digitalRead(CONFIRM_PIN) == LOW) {
    activateMode(modeSelect);
    // Refresh screen upon return
    setMenuScreen(modeSelect);
  }

  else if (digitalRead(RIGHT_PIN) == LOW) {
    // Wait for button to finish being pressed
    while (digitalRead(RIGHT_PIN) == LOW) {}
    // Can't go past the last game
    if (modeSelect < (GAME_COUNT - 1)) {
      ++modeSelect;
    }
    setMenuScreen(modeSelect);
  }

  else if (digitalRead(LEFT_PIN) == LOW) {
    // Wait for button to finish being pressed
    while (digitalRead(LEFT_PIN) == LOW) {}
    // Don't wrap around for now
    if (modeSelect != 0) {
      --modeSelect;
    }
    setMenuScreen(modeSelect);
  }
}
