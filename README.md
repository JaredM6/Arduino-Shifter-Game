# Arduino-Shifter-Game

## Purpose
This is my first solo project dealing with microcontroller programming, circuit design, board design, PCB layout, routing, printing, soldering, and testing. The only real goal of this project is to learn and improve upon what I learned at school. 

The most important thing about this project is that it is simple. My original goal was to make a small handheld game console, most ambitous ideas only come around after many prototypes and earlier versions. This is the first of those prototypes. Once I feel comfortable enough in this whole process (with the validation coming from a working PCB that's identical in function to my breadboard prototype) will I move onto more advanced / complex parts and ideas where I can really get creative (looking at you 2KB of RAM ATMEGA328P).

This is mostly just documentation for my own project, but if you stumble across this and plan to recreate, be aware that there may be incorrect connections and lack of proper design choices in certain areas. Build at your own risk!


## System Components

The idea was to create a small handheld, very simple game system with some LEDS and some buttons. There are 4 main buttons: left, right, confirm, and exit. There is also 8 LEDs and an OLED display. The main controller is an ATMEGA328P. I did not choose this because of it's specs but rather for the reason that the Ardunio IDE makes it much simpler to program, especially for a small project like this. The LEDs are all controlled by the ATMEGA talking to a 8-bit output shift register. I actually started this project becuase I randomly found the shift register in my box of parts and just wondered if I could get it to work. I then expaned upon that and got to this point in the journey.

## Versions

I started out with an Arduino Uno Rev 3 and breadboards to house all the components. After reaching the point of "good enough" I switched out the Arduino for just the plain ATMEGA328P in the breadboard and any of the functional peripherals it needed (crystal, reset line pullup, etc). Up until this point, I had just been placing the ATMEGA into the Arduino, programming it, and re-placing it in the breadboard. 1 broken pin later and many flat-head scratch marks later, I moved to programming via FTDI. With this I was able to cut out the Arduino board altogether (I still use the IDE to program, much easier than the command line alternative). Once all functional, I moved onto the PCB layout since I wanted a complete project. This is where my college experience stopped and my new experience began: PCB design.