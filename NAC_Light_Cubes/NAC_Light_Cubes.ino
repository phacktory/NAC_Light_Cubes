/*

LIGHT CUBES for NACO "Hack the Sympony" - Feb 3 & 5, 2018.
Written by Andrew E. Pelling
pHacktory Inc
GitHub: https://github.com/phacktory/NAC_Light_Cubes

Description of build and hardware: https://medium.com/phacktory

Concert website: https://nac-cna.ca/en/event/15987

HARDWARE REQUIREMENTS: 
- TEENSY 3.6: https://www.pjrc.com/store/teensy36.html
- OCTOWS2811 Adaptor with CAT6 cable: https://www.pjrc.com/store/octo28_adaptor.html
    Pin 2:  LED Strip #1 (Orange)
    Pin 14: LED strip #2 (Blue)     
    Pin 7:  LED strip #3 (Green)
    Pin 8:  LED strip #4 (Brown)    
    Pin 6:  LED strip #5 (Orange)  
    Pin 20: LED strip #6 (Blue)    
    Pin 21: LED strip #7 (Green)
    Pin 5:  LED strip #8 (Brown)
    Pins 3, 4, 15 & 16 - DO NOT USE

LIBRARIES AND CREDITS
  - Bounce2 library found here: 
    https://github.com/thomasfredericks/Bounce2
  - Adafruit Neopixel Library:
    https://github.com/adafruit/Adafruit_NeoPixel
  - Original version of the Neopatterns library:
    https://learn.adafruit.com/multi-tasking-the-arduino-part-3
  - Modified version of the Neopatterns library (Neopatterns.h):
    https://learn.adafruit.com/multi-tasking-the-arduino-part-3
  - MSGEQ7 audio spectrum code adapted from:
    https://imgur.com/a/jYhRk
*/

// LIBRARIES

#include <Adafruit_NeoPixel.h>
#include <Neopatterns.h>
#include <Bounce2.h>

// COMMON USER VARIABLES

#define NUM_BANDS 7                   // Number of audio bands on the MSGEQ7, 
                                      // Number corresponds to the number of Light Cubes
                                      
#define NUM_LEDS 204                  // Total number of LEDs on the WS2812b strips inside 
                                      // each Light Cube
                                      
int defaultBrightness = 128;          // Default brghtness setting of the LEDs (0 to 255)
int numReads = 1;                     // Audio data averaging. Set to 1 for NO averging
const unsigned long loop_dlay = 0;    // loop delay to slow down display update rate

const int noise[] = {75, 90, 90, 90, 90, 105, 100}; // Set this to magnitude of noise for 
                                                    // each band. To determine line noise 
                                                    // set each value to zero and open serial 
                                                    // port while on spectrum mode. Do not 
                                                    // play any audio.

const float gain[] = {1.2, 1.0, 1.0, 1.0, 1.0, 1.5, 2.5}; // Gain for each band. Increase 
                                                          // relative brightness of each 
                                                          // colour to produce even spectrum. 
                                                          // Default is 1.0 for every band.

// NEOPATTERN SETUP

void Cube1Complete();
void Cube2Complete();
void Cube3Complete();
void Cube4Complete();
void Cube5Complete();
void Cube6Complete();
void Cube7Complete();

// Define some NeoPatterns for each of the 7 Light Cubes
NeoPatterns Cube1(NUM_LEDS, 2, NEO_GRB + NEO_KHZ800, &Cube1Complete);
NeoPatterns Cube2(NUM_LEDS, 14, NEO_GRB + NEO_KHZ800, &Cube2Complete);
NeoPatterns Cube3(NUM_LEDS, 7, NEO_GRB + NEO_KHZ800, &Cube3Complete);
NeoPatterns Cube4(NUM_LEDS, 8, NEO_GRB + NEO_KHZ800, &Cube4Complete);
NeoPatterns Cube5(NUM_LEDS, 6, NEO_GRB + NEO_KHZ800, &Cube5Complete);
NeoPatterns Cube6(NUM_LEDS, 20, NEO_GRB + NEO_KHZ800, &Cube6Complete);
NeoPatterns Cube7(NUM_LEDS, 21, NEO_GRB + NEO_KHZ800, &Cube7Complete);

// Array of Light Cubes
NeoPatterns Cubes[7] = {Cube1, Cube2, Cube3, Cube4, Cube5, Cube6, Cube7};

// Setting colors for each freqnecy band
uint32_t bandColor[7] = {
  Cubes[0].Color(255,0,255),   // Violet  - 63 Hz center frequency 
  Cubes[1].Color(0,255,255),   // Indigo  - 160 Hz center frequency 
  Cubes[2].Color(0,0,255),     // Blue    - 400 Hz center frequency 
  Cubes[3].Color(0,225,0),     // Green   - 1000 Hz center frequency 
  Cubes[4].Color(255,255,0),   // Yellow  - 2500 Hz center frequency 
  Cubes[5].Color(255,64,0),    // Orange  - 6250 Hz center frequency 
  Cubes[6].Color(255,0,0)      // Red     - 16000 Hz center frequency 
};

// MSGEQ7 AUDIO ANALYZER SETUP

const byte audioPin           = A9;   // audio data from shield
const byte strobePin          = 1;    // data strobe for shield
const byte resetPin           = 0;    // reset strobe for shield
int mag = 0;          //the magnitude of a freq band
int numON[7];         //the number of LEDs on in a freq band
int bright = 0;       //brightness of LEDSs in each band
float fl_mag = 0.0;   //floating point mag after noise removal and scaling

// BUTTON BOUNCE SETUP

#define BUTTON_PIN_1 17   // All off button pin 
#define BUTTON_PIN_2 18   // Test button pin
#define BUTTON_PIN_3 19   // Ambient button
#define BUTTON_PIN_4 22   // Spectrum button 
Bounce spectrumButton = Bounce(); 
Bounce ambientButton = Bounce(); 
Bounce testButton = Bounce(); 
Bounce offButton = Bounce(); 

// LIGHT CUBE MODE CONTROL

String mode = "spectrum";    // Mode setting
int blinkTest = 0;      // Counter for test() function

void setup() { 
  // Start serial output
  Serial.begin(115200);

  // Power indicator 
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // Setting up the MSGEQ7 IC
  pinMode(resetPin, OUTPUT);
  pinMode(strobePin, OUTPUT);
  digitalWrite(resetPin,HIGH);
  delayMicroseconds(5);
  digitalWrite(strobePin,HIGH);
  delayMicroseconds(50);              
  digitalWrite(strobePin,LOW);
  delayMicroseconds(50);           
  digitalWrite(resetPin,LOW);
  delayMicroseconds(5);               
  digitalWrite(strobePin,HIGH);
  delayMicroseconds(100);               

  // Setup buttons with internal pull-ups
  pinMode(BUTTON_PIN_1,INPUT_PULLUP);
  offButton.attach(BUTTON_PIN_1);
  offButton.interval(5);
  pinMode(BUTTON_PIN_2,INPUT_PULLUP);
  testButton.attach(BUTTON_PIN_2);
  testButton.interval(5);
  pinMode(BUTTON_PIN_3,INPUT_PULLUP);
  ambientButton.attach(BUTTON_PIN_3);
  ambientButton.interval(5);
  pinMode(BUTTON_PIN_4,INPUT_PULLUP);
  spectrumButton.attach(BUTTON_PIN_4);
  spectrumButton.interval(5);

  // Initialize the Light Cubes 
  for (int i = 0; i<7; i++) {
    Cubes[i].begin();
    Cubes[i].Update();
  }
}

void loop() {
  
  setMode();  // Set the operating mode

  if (mode == "spectrum") {
    spectrum();
  } else if (mode == "ambient") {
    ambient();
  } else if (mode == "test") {
    test();
  } else if (mode == "off") {
    off();
  }
  delay(loop_dlay);
}

// GET AUDIO FREQUENCY SPECTRUM 

void spectrum() {
  
  // Collect amplitude data for each frequency band
  for(int band = 0; band < 7; band++) {
    // AVERAGING (set numReads to 1 for no averaging)
    int reads = 0;      // Holds cumulative audio reads
    for (int i = 0; i < numReads; i++) {
      digitalWrite(strobePin, LOW); //set analyzer to low to read
      delayMicroseconds(40);
      reads += analogRead(audioPin);
      digitalWrite(strobePin, HIGH); //set analyzer back to high
    }
    mag = reads/numReads;

    // Convert reading into brighness level after removing noise
    mag = max(0, (mag - noise[band]));            // Creates magnitude of frequency
    
    fl_mag = gain[band] * float(mag);             // Adjusts magnitude for gain
    numON[band] = map(fl_mag, 0, 1024, 0, 255);   // Map to brightness value from 0 to 255
    
    Serial.print("[");
    Serial.print(band);
    Serial.print(", ");
    Serial.print(mag);
    Serial.print("] ");
  }
  Serial.println(); 
  
  // Set brightness for each band
  for (int i = 0; i < 7; i++) {
    Cubes[i].setBrightness(numON[i]);
  }
  
  // Display each band on Light Cubes
  for (int i = 0; i < 7; i++) {
    Cubes[i].ColorSet(bandColor[i]);
  };
}

// AMBIENT LIGHTING FUNCTION

void ambient() {
  
  Serial.println("ambient");
  
  for (int i = 0; i < 7; i++) {
    Cubes[i].Update();
    Cubes[i].setBrightness(defaultBrightness);
  }
  
  // All blue fades
  for (int i = 0; i < 7; i++) {
    Cubes[i].ActivePattern = FADE;
    Cubes[i].TotalSteps = (i+1)*32;
    Cubes[i].Color1 = Cubes[i].Color(255/(4*(i+1)),0,255);
    Cubes[i].Color2 = Cubes[i].Color(16,16,16);
  } 
}

// TEST FUNCTION

void test() {
  
  Serial.println("test");

  for (int i = 0; i < 7; i++) {
    Cubes[i].Update();
  }

  // First time through test loop, start Light Cubes
  if (blinkTest == 0) {
    for (int i = 0; i < 7; i++) {
      Cubes[i].setBrightness(defaultBrightness);
      Cubes[i].ColorSet(Cubes[i].Color(0, 0, 0)); 
    }  
  }

  // Blink each Light Cube in sequence 1 to 7 
  // during first pass through test loop
  if (blinkTest < 1) {
    setMode();
    for (int i = 0; i < 7; i++) {
      if (i == 0) {
        Cubes[i].ColorSet(bandColor[i]);
        Cubes[6].ColorSet(Cubes[6].Color(0, 0, 0));
        setMode();      
      } else if (i > 0) {
        Cubes[i].ColorSet(bandColor[i]);
        Cubes[i-1].ColorSet(Cubes[i-1].Color(0, 0, 0));  
        setMode();    
      }
      delay(750);  
    }
    blinkTest++;
    Cubes[6].ColorSet(Cubes[6].Color(0, 0, 0));      
  }

  // After blinking each Light Cube, show a rainbow pattern 
  // with different speeds in each Light Cube until mode change
  if (blinkTest == 1) {
    for (int i = 0; i<7; i++) {
      Cubes[i].ActivePattern = RAINBOW_ALLPX;
      Cubes[i].TotalSteps = 255;
      Cubes[i].Interval = i*2;
    }
  }
}


// OFF FUNCTION

void off() {
  
  Serial.println("off");

  // Turn all Light Cubes off
  for (int i = 0; i<7; i++) {
    Cubes[i].ColorSet(Cubes[i].Color(0, 0, 0));      
  }
}

// SET MODE FUNCTION

void setMode() {
  spectrumButton.update();
  ambientButton.update();
  testButton.update();
  offButton.update();
    
  int b1 = offButton.read();
  int b2 = testButton.read();
  int b3 = ambientButton.read();
  int b4 = spectrumButton.read();
  
  if ( b1 == LOW && b2 == HIGH && b3 == HIGH && b4 == HIGH) {
    mode = "off";
  } else if ( b1 == HIGH && b2 == LOW && b3 == HIGH && b4 == HIGH) {
    mode = "test";
    blinkTest = 0;
  } else if ( b1 == HIGH && b2 == HIGH && b3 == LOW && b4 == HIGH) {
    mode = "ambient";
  } else if ( b1 == HIGH && b2 == HIGH && b3 == HIGH && b4 == LOW) {
    mode = "spectrum";
  } else { 
    mode = mode;
  }
}


// COMPLETION FUNCTIONS FOR NEOPATTERNS

void Cube1Complete() {
  if (mode == "ambient") {
    Cubes[0].Reverse();
  } else if (mode == "test") {
    Cubes[0].Reverse();
  }
}

void Cube2Complete() {
  if (mode == "ambient") {
    Cubes[1].Reverse();
  } else if (mode == "test") {
    // something
  }
}

void Cube3Complete() {
  if (mode == "ambient") {
    Cubes[2].Reverse();
  } else if (mode == "test") {
    // something
  }
}
  
void Cube4Complete() {
  if (mode == "ambient") {
    Cubes[3].Reverse();
  } else if (mode == "test") {
    Cubes[3].Reverse();
  }
}
  
void Cube5Complete() {
  if (mode == "ambient") {
    Cubes[4].Reverse();
  } else if (mode == "test") {
    // something
  }
}
  
void Cube6Complete() {
  if (mode == "ambient") {
    Cubes[5].Reverse();
  } else if (mode == "test") {
    // something
  }
}
  
void Cube7Complete() {
  if (mode == "ambient") {
    Cubes[6].Reverse();
  } else if (mode == "test") {
    Cubes[6].Reverse();
  }
}
