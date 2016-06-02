/*
  firmware.ino - the main firmware file
  Part of the WaveGlide firmware. See: https://github.com/nortd/WaveGlide
  Copyright (c) 2015 Stefan Hechenberger

  The WaveGlide firmware is free software: you can redistribute it and/or
  modify it under the terms of the GNU General Public License as published
  by the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  The firmware is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the  GNU General Public License
  for more details.
*/

// version
#define VERSION "0.9.0"

// debug switches
#define SIMULATE_ALTITUDE 6000.0

// display pins
#define sclk 13
#define mosi 11
#define dc   8
#define cs   10
#define rst  9

// barometric I2C
#define baro_scl A4
#define baro_sda A5

#define breath A3    // differential pressure sensor
#define button 3     // ui button
#define buzzer 5     // piezo buzzer
#define battery A2   // battery voltage
#define valve 6      // air valve

// Color definitions
// use this python code to generate
// def color16bit(r,g,b):  # r,g,b ... 0-255
//    return hex(((((r >> 3) << 6) | g >> 2) << 5) | b >> 3)
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define OXYBLUE         0x2C3F
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
#define GRAY            0x7BEF
#define DARKGRAY        0x528A
#define ORANGE          0xFC23
#define UIORANGE        0xFD20
#define DARKORANGE      0xFD20
#define NAVY            0x000F
#define DARKGREEN       0x03E0
#define DARKCYAN        0x03EF
#define MAROON          0x7800
#define PURPLE          0x780F
#define OLIVE           0x7BE0
#define LIGHTGRAY       0xC618
#define DARKBLUE        0x0808
#define DARKRED         0x7080
#define GREENYELLOW     0xAFE5
#define PINK            0xF81F

#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"

#include <Wire.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_BMP085_U.h"

extern "C" {
  #include "rhythm.h"
  #include "bitmaps.c"
}

// breathing sensor
uint16_t last_sense_breathing = 0;      // for sample timing
uint16_t last_sense_breathing_dur = 0;  // for samble timing
int breathval = 0;
uint16_t graph_col = DARKGRAY;

// altitude sensor
uint16_t last_sense_altitude = 0;
uint16_t last_sense_altitude_dur = 0;
int pressure = 0;
int temperature = 0;
int altitude = 0;  // pressure alt (bases on 1013 hPa)
float altitude_smoothed = 0;
// barometric sensor via I2C
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

// oxygen setting related
#define METERS2FEET 3.28084
#define OXYGEN_100PCT_FL 300  // FL300
#define OXYGEN_100PCT_ALTITUDE 9144  // FL300
int oxygen_start_fls[] = {80, 50}; // FL80, FL50, CAREFUL: length 2 expected
int oxygen_start_alts[] = {2625, 1524}; // FL80, FL50, CAREFUL: length 2 expected
uint8_t alt_setting = 0;  // used as index in above arrays
int oxygen_pct = 0;

// battery voltage monitoring
uint16_t last_sense_battery = 0;
uint16_t last_sense_battery_dur = 0;
float bat_pct = 0.9;
uint16_t bat_col = WHITE;

// interface
volatile bool button_short_handled = true;
volatile uint16_t last_button = 0;
volatile uint16_t last_button_dur = 0;
bool state = LOW;
// adjustment percentages, applied to oxygen_pct
float adj_pcts[] = {0.7, 1.0, 1.3, 100.0};  // CAREFUL: length 4 expected
uint8_t adj_setting = 0;  // will be 1 after registering interrupt
#define SAMPLES_GRAPH_WIDTH 128
uint8_t samplepos = 0;
char charBuf[50];
// display via SPI, 128x96
Adafruit_SSD1351 tft = Adafruit_SSD1351(cs, dc, rst);



void onButton() {
  state = !state;
  last_button_dur = millis()-last_button;
  last_button = millis();
  if (state && last_button_dur > 40) {  // filter bounces
    button_short_handled = false;
  }
}



void setup(void) {
  analogReference(EXTERNAL);
  pinMode(breath, INPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(valve, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(battery, INPUT);
  // analogWrite(buzzer, 150);

  // check for button held during power up
  bool button_held = false;
  if (!digitalRead(button)) { // pressed
    button_held = true;
  }

  // Serial.begin(9600);
  // Serial.println("init");
  tft.begin();
  //tft.setRotation(1);  // 1: 90 clockwise, 2: 180, 3: 270
  tft.fillScreen(BLACK);

  /* Initialise the barometric sensor */
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    // Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  // testtriangles();
  // tft.invert(true);
  // title animation
  tft.drawXBitmap(20, 76, glider_bits, glider_width, glider_height, WHITE);
  delay(200);
  tft.fillScreen(BLACK);
  tft.drawXBitmap(20, 56, glider_bits, glider_width, glider_height, WHITE);
  tft.drawXBitmap(-60, 65, cloud_left_bits, cloud_left_width, cloud_left_height, WHITE);
  tft.drawXBitmap(133, 65, cloud_right_bits, cloud_right_width, cloud_right_height, WHITE);
  delay(200);
  tft.fillScreen(BLACK);
  tft.drawXBitmap(20, 36, glider_bits, glider_width, glider_height, WHITE);
  tft.drawXBitmap(-40, 65, cloud_left_bits, cloud_left_width, cloud_left_height, WHITE);
  tft.drawXBitmap(113, 65, cloud_right_bits, cloud_right_width, cloud_right_height, WHITE);
  delay(200);
  tft.fillScreen(BLACK);
  tft.drawXBitmap(20, 16, glider_bits, glider_width, glider_height, WHITE);
  tft.drawXBitmap(-20, 65, cloud_left_bits, cloud_left_width, cloud_left_height, WHITE);
  tft.drawXBitmap(93, 65, cloud_right_bits, cloud_right_width, cloud_right_height, WHITE);
  delay(200);
  tft.fillScreen(BLACK);
  tft.drawXBitmap(20, -14, glider_bits, glider_width, glider_height, GRAY);
  tft.drawXBitmap(0, 35, cloud_left_bits, cloud_left_width, cloud_left_height, GRAY);
  tft.drawXBitmap(73, 35, cloud_right_bits, cloud_right_width, cloud_right_height, GRAY);
  delay(200);
  tft.fillScreen(BLACK);
  tft.drawXBitmap(20, -44, glider_bits, glider_width, glider_height, GRAY);
  tft.drawXBitmap(0, 5, cloud_left_bits, cloud_left_width, cloud_left_height, GRAY);
  tft.drawXBitmap(73, 5, cloud_right_bits, cloud_right_width, cloud_right_height, GRAY);
  delay(200);
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  displayText("WAVEGLIDE", 12, 28, ORANGE);
  tft.setTextSize(1);
  displayText("firmware   v", 12, 50, ORANGE);
  displayText(VERSION, 86, 50, WHITE);
  displayText("(beta testing only)", 8, 64, GRAY);
  delay(3000);
  tft.fillScreen(BLACK);

  // build GUI
  //
  displayText("START", 31, 4, DARKBLUE);
  displayText("FL080", 64, 4, OXYBLUE);
  tft.drawLine(94, 7, 127, 7, OXYBLUE);
  // tft.fillRect(124, 8, 4, 48, OXYBLUE);
  tft.fillRect(124, 8, 4, map(oxygen_start_fls[alt_setting], 0, OXYGEN_100PCT_FL, 48, 0), OXYBLUE);
  displayText("300", 110, 0, DARKBLUE);

  adj_draw_rects();
  tft.fillRect(26, 16, 14, 14, UIORANGE);
  adj_draw_symbols();

  displayText("O2", 11, 36, DARKBLUE);
  tft.fillRect(56, 36, 37, 7, OXYBLUE);
  displayText("100%", 28, 36, WHITE);

  displayText("NOW", 11, 52, DARKBLUE);
  tft.setTextSize(2);
  displayText("FL000", 35, 49, WHITE);
  tft.setTextSize(1);
  tft.drawLine(95, 56, 123, 56, WHITE);
  // tft.fillRect(124, 48, 4, 8, WHITE);
  // temperature
  displayText("00", 99, 41, DARKBLUE);
  tft.drawPixel(112, 41, DARKBLUE);
  tft.drawPixel(112, 43, DARKBLUE);
  tft.drawPixel(111, 42, DARKBLUE);
  tft.drawPixel(113, 42, DARKBLUE);
  displayText("0", 123, 57, DARKBLUE);


  // set alt mode
  if (button_held) {
    // If button was helled during power-up set to next alt mode.
    // Specifically this means oxygenation starts at a higher alt.
    alt_setting++;
    if (alt_setting >= 2) { alt_setting = 0; }
    sprintf(charBuf, "FL%03i", oxygen_start_fls[alt_setting]);
    // draw
    if (alt_setting == 0) {
      tft.fillRect(63, 4, 32, 7, BLACK);
      displayText(charBuf, 64, 4, OXYBLUE);
      tft.fillRect(124, 8, 4, 48, OXYBLUE);
    } else if (alt_setting == 1) {
      tft.fillRect(63, 4, 32, 7, OXYBLUE);
      displayText(charBuf, 64, 4, BLACK);
      tft.fillRect(124, 8, 4, 48, BLACK);
      tft.fillRect(124, 8, 4, map(oxygen_start_fls[alt_setting], 0, OXYGEN_100PCT_FL, 48, 0), OXYBLUE);
    }
  }

  // enable button interrupt
  attachInterrupt(digitalPinToInterrupt(button), onButton, CHANGE);
}



void loop() {
  last_sense_breathing_dur = millis()-last_sense_breathing;
  if (last_sense_breathing_dur > RHYTHM_TEMPRES) {
    sense_breathing();
    last_sense_breathing = millis();
  }

  last_sense_altitude_dur = millis()-last_sense_altitude;
  if (last_sense_altitude_dur > RHYTHM_TEMPRES*20) {
    sense_altitude();
    last_sense_altitude = millis();
  }

  last_sense_battery_dur = millis()-last_sense_battery;
  if (last_sense_battery_dur > RHYTHM_TEMPRES*30) {
    sense_battery();
    last_sense_battery = millis();
  }

  // handle short button press
  if (!button_short_handled) {
    handle_short_button();
    button_short_handled = true;
  }
}



void sense_breathing() {
  breathval = analogRead(breath);
  rhythm_addval(breathval);
  // Serial.println(breathval);
  int val = (breathval-rhythm_get_baseline())*0.4;
  if (val > 12) { val = 12; }
  if (val < -12) { val = -12; }
  if (!baseline_set()) { graph_col = DARKBLUE; }
  tft.drawLine(samplepos, 79, samplepos, 79-val, graph_col);
  if (samplepos+1 < tft.width()) {
   tft.drawLine(samplepos+1, 93, samplepos+1, 64, BLACK);
  } else {
   tft.drawLine(0, 93, 0, 64, BLACK);
  }
  if (++samplepos == SAMPLES_GRAPH_WIDTH) { samplepos = 0; } // inc, wrap

  if (rhythm_oxygen(oxygen_pct)) {
    digitalWrite(valve, HIGH);
    graph_col = OXYBLUE;
  } else {
    digitalWrite(valve, LOW);
    graph_col = DARKGRAY;
  }

  // display period
  tft.drawLine(0, 94, tft.width(), 94, BLACK);
  tft.drawLine(0, 94, rhythm_get_period(), 94, DARKBLUE);

  // display phase
  tft.drawLine(0, 95, tft.width(), 95, BLACK);
  tft.drawLine(0, 95, rhythm_get_phase(), 95, DARKGRAY);

}



void sense_altitude() {
  sensors_event_t event;
  bmp.getEvent(&event);

  // Display the results (barometric pressure is measure in hPa)
  if (event.pressure) {
    pressure = round(event.pressure);
    float temp;
    bmp.getTemperature(&temp);
    temperature = round(temp);
    // print numeral
    sprintf(charBuf, "%02i", constrain(temperature, 0, 99));
    tft.fillRect(99, 41, 12, 7, BLACK);
    displayText(charBuf, 99, 41, DARKBLUE);
    // Serial.print("Temperature: ");
    // Serial.print(temp);
    // Serial.println(" C");

    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    altitude = round(bmp.pressureToAltitude(seaLevelPressure,
                                            event.pressure,
                                            temperature));
    if (altitude < altitude_smoothed + 20000
        && altitude > altitude_smoothed - 1000) {
      #ifdef SIMULATE_ALTITUDE
      altitude_smoothed = SIMULATE_ALTITUDE;
      #else
      altitude_smoothed = 0.8*altitude_smoothed + 0.2*altitude;
      #endif
      set_oxygen_pct(altitude_smoothed);
    } else {
      // reject
    }
    // print numeral, flight level, in feet, /100, rounded to nearest 500
    // int flight_level = round((altitude_smoothed * METERS2FEET)/500) * 5;
    // flight level, do not round to nearest 500
    int flight_level = round((altitude_smoothed * METERS2FEET)/100);
    flight_level = constrain(flight_level, 0, OXYGEN_100PCT_FL);
    sprintf(charBuf, "%03i", flight_level);
    tft.fillRect(59, 49, 36, 14, BLACK);
    tft.setTextSize(2);
    displayText(charBuf, 59, 49, WHITE);
    tft.setTextSize(1);
    tft.fillRect(120, 8, 4, 48, BLACK);
    int flheight = map(flight_level, 0, OXYGEN_100PCT_FL, 0, 48);
    tft.fillRect(120, 8+(48-flheight), 4, flheight, WHITE);
  } else {
    // Serial.println("Sensor error");
  }
}



void set_oxygen_pct(float alt) {
  // required oxygen by altitude see ch.12 "Dancing with the Wind" by Clement
  // FL000 - 21%
  // FL050 - 25%
  // FL100 - 30%
  // FL150 - 37%
  // FL200 - 45%
  // FL250 - 56%
  // FL300 - 70%
  //
  // Let's assume the following:
  // - nose cannula cannot supply more than 70% tops
  // - average tidal lung volume is 500ml, https://en.wikipedia.org/wiki/Tidal_volume
  // - the average inhale freq is 12/min or 0.2Hz
  // - the average inhalation volume is 6l/min, regulator is set to it
  //
  // The question is how long (pct of inhalation time) needs to be oxygenated
  // full (70%) to achieve the oxygen levels of the above table.
  // 70*(pct/100.0) + 21*(1.0-(pct/100.0)) =  required_o2_pct
  //
  // Oxygenation times:
  // FL000 -    0m -   0%
  // FL050 - 1524m -   8%
  // FL100 - 3048m -  19%
  // FL150 - 4572m -  33%
  // FL200 - 6096m -  49%
  // FL250 - 7620m -  72%
  // FL300 - 9144m - 100%
  //
  // Need function to produce these factors:
  //    0 -> 0
  // 1524 -> 0.00525
  // 3048 -> 0.00623
  // 4572 -> 0.00722
  // 6096 -> 0.00803
  // 7620 -> 0.00945
  // 9144 -> 0.01094
  //
  // Fitting these values:
  // [[1524,0.00525], [3048,0.00623], [4572,0.00722], [6096,0.00803], [7620,0.00945], [9144,0.01094]]
  // came up with this (using the "Fit" command on WolframAlpha):
  // (0.0000008*x**2+ 0.0036*x)
  // (0.00000084*x**2+ 0.00347*x)-2 - norm curve (less for initial O2 bursts)
  // (0.00000134*x**2+ 0.0036*x) - high curve
  // ((0.00000180*(x-1400)**2)+3) - low curve

  if (adj_setting == 3) {  // max setting -> 100%
    oxygen_pct = 100;
  } else {
    // set a oxygen percentaged based on altitude
    if (alt > oxygen_start_alts[alt_setting]) {
      if (alt < OXYGEN_100PCT_ALTITUDE) {
        // oxygen_pct = map(alt, 0, OXYGEN_100PCT_ALTITUDE, 0, 100); // linearly
        // oxygen_pct = (0.0000008*alt + 0.0036)*alt; // above function
        oxygen_pct = ((0.00000180*(alt-1400)*(alt-1400))+3);  // above function
      } else {
        oxygen_pct = 100; // 100%
      }
    } else {
      oxygen_pct = 0; // 0%, below start altitude
    }
    // apply adjustement setting
    oxygen_pct = oxygen_pct * adj_pcts[adj_setting];
    oxygen_pct = constrain(oxygen_pct, 0, 100);
  }
  // Display
  tft.fillRect(28, 36, 18, 7, BLACK);
  int pctwidth = map(oxygen_pct, 0, 100, 0, 35);
  if (oxygen_pct < 100) {
    sprintf(charBuf, "%02i", oxygen_pct);
    displayText(charBuf, 34, 36, WHITE);
    tft.fillRect(57, 37, pctwidth, 5, OXYBLUE);
    tft.fillRect(57+pctwidth, 37, 35-pctwidth, 5, BLACK);
  } else if (oxygen_pct >= 100) {
    sprintf(charBuf, "%i", oxygen_pct);
    displayText(charBuf, 28, 36, WHITE);
    tft.fillRect(57, 37, 35, 5, OXYBLUE);
  }
}



void sense_battery() {
  // AREF used, 0-5.09 -> 0-1023
  // 3.2V low-battery, map this to 0
  // 3.7 to max
  float batvolts = analogRead(battery) * (5.04/1023.0); // map to 0-5.09
  float newbat_pct = 0.0;
  if (batvolts <= 3.2) {
    newbat_pct = 0.0;
  } else if (batvolts >= 4.1) {
    newbat_pct = 1.0;
  } else {
    newbat_pct = (batvolts - 3.2) * 1.11;  // * 1/(4.1-3.2)
  }
  // smooth in
  bat_pct = 0.8*bat_pct + 0.2*newbat_pct;
  // color
  if (bat_pct > 0.5) {
    bat_col = WHITE;
  } else if (bat_pct > 0.3) {
    bat_col = DARKORANGE;
  } else {
    bat_col = RED;
  }
  // print numeral
  // sprintf(charBuf, "%03i", round(bat_pct*100));
  // tft.fillRect(95, 21, 6*4, 7, BLACK);
  // displayText(charBuf, 95, 21, bat_col);
  // displayText("%", 95+18, 21, bat_col);

  // print symbol
  tft.fillRect(0, 0, 23, 10, BLACK);
  // outline
  tft.drawLine(0, 0, 20, 0, bat_col);
  tft.drawLine(20, 1, 20, 8, bat_col);
  tft.fillRect(21, 3, 2, 4, bat_col);
  tft.drawLine(20, 9, 0, 9, bat_col);
  tft.drawLine(0, 8, 0, 1, bat_col);
  // charge level
  uint8_t level = round(bat_pct*20);
  tft.fillRect(1, 1, level, 8, bat_col);
}



void handle_short_button() {
  adj_setting++;
  if (adj_setting >= 4) { adj_setting = 0; }
  // draw
  if (adj_setting == 0) {
    adj_draw_rects();
    tft.fillRect(10, 16, 14, 14, UIORANGE);
    adj_draw_symbols();
  } else if (adj_setting == 1) {
    adj_draw_rects();
    tft.fillRect(26, 16, 14, 14, UIORANGE);
    adj_draw_symbols();
  } else if (adj_setting == 2) {
    adj_draw_rects();
    tft.fillRect(42, 16, 14, 14, UIORANGE);
    adj_draw_symbols();
  } else if (adj_setting == 3) {
    adj_draw_rects();
    tft.fillRect(83, 16, 29, 14, UIORANGE);
    adj_draw_symbols();
  }
  // update oxygen_pct
  set_oxygen_pct(altitude_smoothed);
}

void adj_draw_rects() {
  tft.fillRect(10, 16, 14, 14, DARKBLUE);
  tft.fillRect(26, 16, 14, 14, DARKBLUE);
  tft.fillRect(42, 16, 14, 14, DARKBLUE);
  tft.fillRect(83, 16, 29, 14, DARKBLUE);
}

void adj_draw_symbols() {
  // 0
  tft.fillRect(13, 21, 8, 4, BLACK);
  // 1
  tft.drawLine(32, 18, 33, 18, BLACK);
  tft.drawLine(31, 19, 34, 19, BLACK);
  tft.drawLine(30, 20, 35, 20, BLACK);
  tft.drawLine(29, 21, 36, 21, BLACK);
  tft.drawLine(28, 22, 37, 22, BLACK);
  tft.drawLine(28, 23, 37, 23, BLACK);
  tft.drawLine(29, 24, 36, 24, BLACK);
  tft.drawLine(30, 25, 35, 25, BLACK);
  tft.drawLine(31, 26, 34, 26, BLACK);
  tft.drawLine(32, 27, 33, 27, BLACK);
  // 2
  tft.fillRect(47, 18, 3, 3, BLACK);
  tft.fillRect(44, 21, 10, 4, BLACK);
  tft.fillRect(47, 25, 4, 3, BLACK);
  // 3
  tft.fillRect(60, 21, 19, 4, BLACK);
  tft.fillRect(63, 18, 4, 3, BLACK);
  tft.fillRect(72, 18, 4, 3, BLACK);
  tft.fillRect(63, 25, 4, 3, BLACK);
  tft.fillRect(72, 25, 4, 3, BLACK);
  // 4
    // M
  tft.fillRect(85, 18, 2, 10, BLACK);
  tft.fillRect(92, 18, 2, 10, BLACK);
  tft.drawLine(87, 19, 87, 21, BLACK);
  tft.drawLine(88, 20, 88, 22, BLACK);
  tft.drawLine(89, 21, 89, 23, BLACK);
  tft.drawLine(90, 20, 90, 22, BLACK);
  tft.drawLine(91, 19, 91, 21, BLACK);
    // A
  tft.fillRect(95, 20, 2, 8, BLACK);
  tft.fillRect(100, 20, 2, 8, BLACK);
  tft.fillRect(97, 23, 3, 2, BLACK);
  tft.drawLine(96, 19, 100, 19, BLACK);
  tft.drawLine(97, 18, 99, 18, BLACK);
  tft.drawPixel(97, 20, BLACK);
  tft.drawPixel(99, 20, BLACK);
    //X
  tft.fillRect(103, 18, 2, 2, BLACK);
  tft.fillRect(108, 18, 2, 2, BLACK);
  tft.fillRect(104, 20, 2, 2, BLACK);
  tft.fillRect(107, 20, 2, 2, BLACK);
  tft.fillRect(104, 24, 2, 2, BLACK);
  tft.fillRect(107, 24, 2, 2, BLACK);
  tft.fillRect(103, 26, 2, 2, BLACK);
  tft.fillRect(108, 26, 2, 2, BLACK);
  tft.fillRect(105, 22, 3, 2, BLACK);
}



void displayText(char *text, int16_t x, int16_t y, uint16_t color) {
  tft.setCursor(x,y);
  tft.setTextColor(color);
  tft.print(text);
}
