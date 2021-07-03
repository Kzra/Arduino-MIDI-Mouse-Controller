// Arduino MIDI PS/2 Computer Mouse Device 
// Requires Adafruit LED I2C 8 * 8 Bicolour matrix
// Adafruit I2C 0.96" OLED Display 
// PS/2 Mouse adapter
// PS/2 Roller Ball Mouse 
// Arduino Micro (or any Arduino with native USB capabilities)
//
// Version 1.0.0 
// Ezra Kitson 2021 MIT (C) 

// Import libraries
#include "PS2Mouse.h" // Used to read PS/2 output
#include <Wire.h>
#include "Adafruit_LEDBackpack.h" // Used for LED matrix
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // Used for OLED display
#include "MIDIUSB.h" // Used to send MIDI from Arduino

// OLED Display 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// PS/2 Adaptor
#define DATA_PIN 6
#define CLOCK_PIN 5
PS2Mouse mouse(CLOCK_PIN, DATA_PIN);

// LED matrix 
Adafruit_BicolorMatrix matrix = Adafruit_BicolorMatrix();

// PS/2 mouse constants and variables
// X-Axis
int x_abs = 0; 
int x_temp = 0;
const int x_upper = 1000;
const int x_lower = -1000;
// Y-Axis
int y_abs = 0;
int y_temp = 0;
const int y_upper = 250; 
const int y_lower = -250;
// Z-Axis (Mouse Wheel)
int z_abs = 0;
int z_temp = 0;
const int z_upper = 32;
const int z_lower = 0;

// MIDI and UI constants and variables
int midiVelocity;
int velMap; 
int midiPitch;
int noteVal;
int octave;
int midiWheel;
int cLED;
int c2LED = 3;
char const notes[] = {'C','C','D','D','E','F','F','G','G','A','A','B'};
char const sharps[] = {' ','#',' ','#',' ',' ','#',' ','#',' ','#',' '};
bool intMode = false;  
bool lclick = false; 
bool rclick = false; 
int lnoteVal; 
int rnoteVal; 
int lvelMap;
int rvelMap;
int lMidiPitch; 
int rMidiPitch;
int lMidiVel;
int rMidiVel;
int prevMidiPitch;
int prevMidiWheel;
const int midiChannel = 0; // THIS DETERMINES THE MIDI OUTPUT CHANNEL
const int midiControl = 1; // THIS DETERMINES THE CONTINOUS CONTROLLER ASSOCIATED WITH THE MOUSE WHEEL

void setup() {
  Serial.begin(9600);
  // Initialise the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay(); // Clear the buffer
  display.setTextColor(SSD1306_WHITE,BLACK); // Draw white text on a black background (so it overwrites)
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.display();   // Show the display buffer on the screen. 
  mouse.initialize(); // Initialise the PS/2 adapter
  matrix.begin(0x70);  // Initialise the LED matrix
  matrix.setRotation(3); // rotate the LED matrix so pins are at 0th row 
  Serial.println("Mouse, OLED and LED matrix initialized");
}

void loop() {
    // Read the X,Y and Z mouse data
    MouseData data = mouse.readData();    
    cLED = 3;
    x_temp = x_abs + data.position.x;
    if (x_temp <= x_upper and x_temp >= x_lower){
      x_abs = x_temp;
    }
    y_temp = y_abs + data.position.y;
    if (y_temp <= y_upper and y_temp >= y_lower){
      y_abs = y_temp;
    }
    z_temp = z_abs + data.wheel;
    if (z_temp <= z_upper and z_temp >= z_lower){
      z_abs = z_temp;
    }
    // Map mouse values to MIDI range (0-127)
    midiPitch = map(x_abs,x_lower,x_upper,0,127);
    midiVelocity = map(y_abs,y_lower,y_upper,0,127);
    midiWheel = map(z_abs,z_lower,z_upper,0,127);
    // Get the note value relative to C major
    noteVal = midiPitch % 12;
    // Get the octave relative to middle C (MIDI 48)
    octave = floor(midiPitch/12) - 4; 
    // Map the velocity to an LED matrix coordinate (between 0 and 7)
    velMap = map(y_abs,y_lower,y_upper-50,7,0);
    // If there is a wheel click 
    if (bitRead(data.status, 2) == 1){
      if(intMode == true){
        intMode = false;
      } else {
        intMode = true;
      }
      delay(100);
    }   
    // send continuous controller message
    if(midiWheel != prevMidiWheel){
      controlChange(midiChannel,midiControl,midiWheel);
      prevMidiWheel = midiWheel;
    }
    // update the LED matrix and send MIDI messages
    matrix.clear();
    //Interval Mode 
    // -> left and right clicks map to different notes
    // -> it is possible to play multiple notes at once 
    // -> moving the mouse after clicking down does not change the note 
    // -> matrix pixel turns red for left click, right for orange click
    if(intMode == true){ 
      // left click
      if (bitRead(data.status, 0) == 1){
         if (lclick == false){          
          lnoteVal = noteVal; 
          lvelMap = velMap;
          lMidiPitch = midiPitch;
          lMidiVel = midiVelocity;
          noteOn(midiChannel, lMidiPitch, lMidiVel);
          MidiUSB.flush(); // MIDI flush is required to ensure messages are sent instantly
         } 
         cLED = 1;
         lclick = true;
         switch(lnoteVal){
          case 0: 
          matrix.drawPixel(0, lvelMap, cLED);
          matrix.drawPixel(7, lvelMap, cLED);
          break;
          case 1: 
          matrix.drawPixel(0, lvelMap, cLED);
          matrix.drawPixel(1, lvelMap, cLED);
          break;
          case 2: 
          matrix.drawPixel(1, lvelMap, cLED);
          break;
          case 3: 
          matrix.drawPixel(1, lvelMap, cLED);
          matrix.drawPixel(2, lvelMap, cLED);
          break;
          case 4: 
          matrix.drawPixel(2, lvelMap, cLED);
          break;
          case 5: 
          matrix.drawPixel(3, lvelMap, cLED);
          break;
          case 6: 
          matrix.drawPixel(3, lvelMap, cLED);
          matrix.drawPixel(4, lvelMap, cLED);
          break;   
          case 7: 
          matrix.drawPixel(4, lvelMap, cLED);
          break; 
          case 8: 
          matrix.drawPixel(4, lvelMap, cLED);
          matrix.drawPixel(5, lvelMap, cLED);
          break; 
          case 9: 
          matrix.drawPixel(5, lvelMap, cLED);
          break;    
          case 10: 
          matrix.drawPixel(5, lvelMap, cLED);
          matrix.drawPixel(6, lvelMap, cLED);
          break;
          case 11: 
          matrix.drawPixel(6, lvelMap, cLED);
          break;                                                        
        }
      } else {
        if (lclick == true){
          lclick = false;
          // send midi note off 
          noteOff(midiChannel, lMidiPitch, lMidiVel);
          MidiUSB.flush();  
        }
      }
      // right click 
      if (bitRead(data.status,1) == 1){
         if (rclick == false){         
          rnoteVal = noteVal; 
          rvelMap = velMap;
          rMidiPitch = midiPitch;
          rMidiVel = midiVelocity;
          noteOn(midiChannel, rMidiPitch, rMidiVel);
          MidiUSB.flush();
         }
         cLED = 2;
         rclick = true;
         switch(rnoteVal){
          case 0: 
          matrix.drawPixel(0, rvelMap, cLED);
          matrix.drawPixel(7, rvelMap, cLED);
          break;
          case 1: 
          matrix.drawPixel(0, rvelMap, cLED);
          matrix.drawPixel(1, rvelMap, cLED);
          break;
          case 2: 
          matrix.drawPixel(1, rvelMap, cLED);
          break;
          case 3: 
          matrix.drawPixel(1, rvelMap, cLED);
          matrix.drawPixel(2, rvelMap, cLED);
          break;
          case 4: 
          matrix.drawPixel(2, rvelMap, cLED);
          break;
          case 5: 
          matrix.drawPixel(3, rvelMap, cLED);
          break;
          case 6: 
          matrix.drawPixel(3, rvelMap, cLED);
          matrix.drawPixel(4, rvelMap, cLED);
          break;   
          case 7: 
          matrix.drawPixel(4, rvelMap, cLED);
          break; 
          case 8: 
          matrix.drawPixel(4, rvelMap, cLED);
          matrix.drawPixel(5, rvelMap, cLED);
          break; 
          case 9: 
          matrix.drawPixel(5, rvelMap, cLED);
          break;    
          case 10: 
          matrix.drawPixel(5, rvelMap, cLED);
          matrix.drawPixel(6, rvelMap, cLED);
          break;
          case 11: 
          matrix.drawPixel(6, rvelMap, cLED);
          break;                                                        
        }
      } else {
        if (rclick == true){
          rclick = false;
          // send midi note off 
          noteOff(midiChannel, rMidiPitch, rMidiVel);
          MidiUSB.flush(); 
        }
      } 
    // we only want to return to green LED if the new coordinates are different from the click of our last coordinates 
    if (bitRead(data.status, 0) == 1 and noteVal == lnoteVal and velMap == lvelMap){
      cLED = 1; 
    } else if (bitRead(data.status, 1) == 1 and noteVal == rnoteVal and velMap == rvelMap){
      cLED = 2;
    } else {
      cLED = 3;
    }
    } else { 
        // Glissando Mode
        // -> left and right clicks send the same note 
        // -> if the note pitch has changed turn off the preceeding note 
        // -> it is only possible to play one note at a time
        if (midiPitch != prevMidiPitch){
          noteOff(midiChannel, prevMidiPitch, midiVelocity);
          MidiUSB.flush();
          prevMidiPitch = midiPitch;
        }
        // left click
        if (bitRead(data.status, 0) == 1){
          cLED = 1;
          // note on midiMessage 
          noteOn(midiChannel, midiPitch, midiVelocity);
          MidiUSB.flush();
        } else if (bitRead(data.status,1) == 1){
          cLED = 1;
          // note on midiMessage
          noteOn(midiChannel, midiPitch, midiVelocity);
          MidiUSB.flush();
        } else {
          // note off midiMessage 
          noteOff(midiChannel, midiPitch, midiVelocity);
          MidiUSB.flush();
        }
    }     
    switch(noteVal){
      case 0: 
      matrix.drawPixel(0, velMap, cLED);
      matrix.drawPixel(7, velMap, cLED);
      break;
      case 1: 
      matrix.drawPixel(0, velMap, cLED);
      matrix.drawPixel(1, velMap, cLED);
      break;
      case 2: 
      matrix.drawPixel(1, velMap, cLED);
      break;
      case 3: 
      matrix.drawPixel(1, velMap, cLED);
      matrix.drawPixel(2, velMap, cLED);
      break;
      case 4: 
      matrix.drawPixel(2, velMap, cLED);
      break;
      case 5: 
      matrix.drawPixel(3, velMap, cLED);
      break;
      case 6: 
      matrix.drawPixel(3, velMap, cLED);
      matrix.drawPixel(4, velMap, cLED);
      break;   
      case 7: 
      matrix.drawPixel(4, velMap, cLED);
      break; 
      case 8: 
      matrix.drawPixel(4, velMap, cLED);
      matrix.drawPixel(5, velMap, cLED);
      break; 
      case 9: 
      matrix.drawPixel(5, velMap, cLED);
      break;    
      case 10: 
      matrix.drawPixel(5, velMap, cLED);
      matrix.drawPixel(6, velMap, cLED);
      break;
      case 11: 
      matrix.drawPixel(6, velMap, cLED);
      break;                                                        
    }
    // write to OLED display
    display.setCursor(8, 0);     // Start at top-left corner
    display.setTextSize(4);      // 4:1 pixel scale
    display.write(notes[noteVal]); //current note
    display.write(sharps[noteVal]); //current sharps
    display.setCursor(70,0); // middle of screen top row
    if(octave >= 0 ){
    display.write("+");
    } 
    display.print(octave); // current octave
    display.setCursor(0,32); // second row left side 
    display.setTextSize(2); // 2:1 pixel scale  
    display.write("V:");
    display.print(midiVelocity);
    if(midiVelocity < 100){
      display.write(" ");
    }
    display.setCursor(64,48);
    display.write("W:");
    display.print(midiWheel);
    if(midiVelocity < 100){
      display.write(" ");
    }
    display.setCursor(64,32);
    display.write("M:");
    if(intMode == true){
      display.write("Int");
    } else{
      display.write("Gls");
    }
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setCursor(0,48);
    display.write("MIDI mouse");
    display.setCursor(0,56);
    display.write("Kzra 2021");
    // update the LED and OLED displays
    display.display();
    matrix.writeDisplay();  
    // small delay
    delay(5);
}

// ****
// The following MIDI send functions are taken from https://www.arduino.cc/en/Tutorial/MidiDevice
// ****
// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

void noteOn(byte channel, byte pitch, byte velocity) {

  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};

  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {

  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};

  MidiUSB.sendMIDI(noteOff);
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void controlChange(byte channel, byte control, byte value) {

  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};

  MidiUSB.sendMIDI(event);
}
