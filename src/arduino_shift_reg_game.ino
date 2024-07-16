#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


/*
 *  Input Pins
 */
// *Note* confirm button is connected to pin 2 since only pins 2 and 3 have ISR.
int confirmButton = 2;
int rightSelectButton = 8;
int leftSelectButton = 9;


/*
 *  Output Pins
 */
// Pin connected to DS of 74HC595
int dataPin = 7;
// Pin connected to SH_CP of 74HC595
int clockPin = 3;
// Pin connected to ST_CP of 74HC595
int latchPin = 4;

// Setup for display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Mode selected by user. 0 is default, increase by 1 to the right and wrap around when going left
uint8_t modeSelect = 0;
String modeName = "Random Game!";


/*
 * Custom Functions
 */
 
// Send the data to the shifter
int ledControl(int number)
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
int setOled(String output, int textSize, int xLoc=0, int yLoc=10)
{
  display.clearDisplay();
  // Originally 0,10
  display.setCursor(xLoc, yLoc);
  display.setTextSize(textSize);
  display.println(output);
  display.display();
}

// Activate the chosen mode for the user
int activateMode(uint8_t modeSelect)
{
  switch(modeSelect)
  {
    // Random game
    case 0:
      setOled("Random game!", 2);
      break;
    
    // Count 256
    case 1:
      String number;
      for (int numberToDisplay = 0; numberToDisplay < 256; numberToDisplay++) 
      {
        ledControl(numberToDisplay);
        number = String(numberToDisplay);
        setOled("Number: " + number + "\n\n(Press confirm to exit)", 2); 
        // pause before next value:
        delay(500);
      }
      break;

     // Stoplight game
     case 2:
      setOled("Stoplight! TODO", 2);
      break;
  }
}

// TODO
int shiftOutput(int numberToDisplay, int buttonPressed)
{
  // count from 0 to 255 and display the number
  // on the LEDs
  for (int numberToDisplay = 0; numberToDisplay < 256; numberToDisplay++) 
  {
     
                                              
    
    // pause before next value:
    
    delay(500);
  }

  for (int i = 0; i < 10; i++)
  {
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0b10000001);
    digitalWrite(latchPin, HIGH);
    delay(100);
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0b01000010);
    digitalWrite(latchPin, HIGH);
    delay(100);
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0b00100100);
    digitalWrite(latchPin, HIGH);
    delay(100);
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0b00011000);
    digitalWrite(latchPin, HIGH);
    delay(100);
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0b00100100);
    digitalWrite(latchPin, HIGH);
    delay(100);
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0b01000010);
    digitalWrite(latchPin, HIGH);
    delay(100);
  }
}

/*
 * Arduino Functions (setup and loop)
 */
void setup() 
{
  // Inputs
  pinMode(confirmButton, INPUT_PULLUP);
  pinMode(rightSelectButton, INPUT_PULLUP);
  pinMode(leftSelectButton, INPUT_PULLUP);
  
  // Outputs
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

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
    ledControl(0b00011000);
    activateMode(modeSelect);
  }

  else if (digitalRead(rightSelectButton) == LOW)
  {
    ledControl(0b00001111);
    modeName = "Count 256!";
    // Wait for button to finish being pressed
    while (digitalRead(rightSelectButton) == LOW) {}
    modeSelect++;
  }

  else if (digitalRead(leftSelectButton) == LOW)
  {
    ledControl(0b11110000);
    setOled("Left!", 3);
    // Wait for button to finish being pressed
    while (digitalRead(leftSelectButton) == LOW) {}
    modeSelect--;
  }

  else
  {
    String modeNumber;
    modeNumber = modeSelect;
    // Menu Message
    setOled("    " + modeName + "\n\n<- L  Confirm  R ->\n\n" + modeNumber, 1);
    ledControl(modeSelect);
  }  
}
