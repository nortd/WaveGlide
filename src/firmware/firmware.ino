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

// display pins
#define sclk 13
#define mosi 11
#define dc   8
#define cs   10
#define rst  9

// barometric I2C
#define baro_scl A4
#define baro_sda A5

// differential pressure sensor pin
#define breath A3

// other
#define button 3
#define buzzer 5
#define battery A2

// air valve
#define valve 6

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
#define GRAY            0x8888
#define ORANGE          0xFD20

/* some RGB color definitions                                                 */
// #define Black           0x0000      /*   0,   0,   0 */
// #define Navy            0x000F      /*   0,   0, 128 */
// #define DarkGreen       0x03E0      /*   0, 128,   0 */
// #define DarkCyan        0x03EF      /*   0, 128, 128 */
// #define Maroon          0x7800      /* 128,   0,   0 */
// #define Purple          0x780F      /* 128,   0, 128 */
// #define Olive           0x7BE0      /* 128, 128,   0 */
// #define LightGrey       0xC618      /* 192, 192, 192 */
// #define DarkGrey        0x7BEF      /* 128, 128, 128 */
// #define Blue            0x001F      /*   0,   0, 255 */
// #define Green           0x07E0      /*   0, 255,   0 */
// #define Cyan            0x07FF      /*   0, 255, 255 */
// #define Red             0xF800      /* 255,   0,   0 */
// #define Magenta         0xF81F      /* 255,   0, 255 */
// #define Yellow          0xFFE0      /* 255, 255,   0 */
// #define White           0xFFFF      /* 255, 255, 255 */
// #define Orange          0xFD20      /* 255, 165,   0 */
// #define GreenYellow     0xAFE5      /* 173, 255,  47 */
// #define Pink                        0xF81F

#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"

#include <Wire.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_BMP085_U.h"

extern "C" {
  #include "rhythm.h"
}

uint16_t last_sense_breathing = 0;
uint16_t last_sense_breathing_dur = 0;
int breathval = 0;
int breathval_prev = 1023;
bool oxy_on = false;
uint16_t graph_col = WHITE;

uint16_t last_sense_altitude = 0;
uint16_t last_sense_altitude_dur = 0;
int pressure = 0;
int temperature = 0;
int altitude = 0;  // pressure alt (bases on 1013 hPa)
float altitude_smoothed = 0;

#define OXYGEN_100PCT_ALTITUDE 10058  // FL330
#define OXYGEN_MIN_PCT 0.1  // 0.0 to 1.0, typically 0.1 to 0.3
int oxygen_start_alts[] = {1525, 3050, 3812, 4270}; // FL50, 100, 125, 140
float oxygen_pct = 0.0;
// int oxygen_start_alt = 3812;
int oxygen_start_alt = 0;

int buttonState = 0;
char charBuf[50];

uint8_t bat_pct = 50;
uint16_t bat_col = GREEN;


// Option 1: use any pins but a little slower
//Adafruit_SSD1351 tft = Adafruit_SSD1351(cs, dc, mosi, sclk, rst);

// Option 2: must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
Adafruit_SSD1351 tft = Adafruit_SSD1351(cs, dc, rst);

// barometric sensor via I2C
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);



void setup(void) {
  analogReference(EXTERNAL);
  pinMode(breath, INPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(valve, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(battery, INPUT);
  // analogWrite(buzzer, 150);

  // Serial.begin(9600);
  // Serial.println("init");
  tft.begin();
  //tft.setRotation(1);  // 1: 90 clockwise, 2: 180, 3: 270

  /* Initialise the barometric sensor */
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    // Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  uint16_t time = millis();
  tft.fillRect(0, 0, 128, 96, BLACK);
  time = millis() - time;

  // Serial.println(time, DEC);
  delay(500);

  lcdTestPattern();
  delay(500);
  tft.invert(true);
  delay(100);
  tft.invert(false);
  delay(100);

  // init display
  tft.fillScreen(BLACK);

  displayText("START", 0, 3, GRAY);
  tft.setTextSize(2);
  displayText("FL100", 35, 0, CYAN);
  tft.setTextSize(1);
  tft.drawLine(94, 7, 127, 7, CYAN);
  tft.fillRect(122, 8, 6, 31, CYAN);
  displayText("330", 110, 0, GRAY);

  displayText("ADJ", 12, 21, GRAY);
  tft.fillRect(35, 20, 13, 10, GRAY);
  tft.fillRect(49, 20, 9, 10, GRAY);
  tft.fillRect(59, 20, 10, 10, RED);
  tft.fillRect(70, 20, 10, 10, GRAY);
  tft.fillRect(81, 20, 12, 10, GRAY);

  displayText("O2", 16, 36, GRAY);
  tft.fillRect(56, 36, 37, 7, CYAN);
  displayText("100%", 30, 36, WHITE);

  displayText("NOW", 11, 52, GRAY);
  tft.setTextSize(2);
  displayText("FL006", 35, 49, WHITE);
  tft.setTextSize(1);
  tft.drawLine(94, 56, 127, 56, WHITE);
  tft.fillRect(124, 48, 4, 8, WHITE);
  displayText("32", 97, 57, GRAY);
  tft.drawPixel(110, 57, GRAY);
  tft.drawPixel(110, 59, GRAY);
  tft.drawPixel(109, 58, GRAY);
  tft.drawPixel(111, 58, GRAY);
  displayText("0", 123, 57, GRAY);
}

void loop() {
  last_sense_breathing_dur = millis()-last_sense_breathing;
  if (last_sense_breathing_dur > RHYTHM_TEMPRES) {
    // tft.fillRect(6, 32, 4*6, 32, BLACK);
    // sprintf(charBuf, "[%i]", last_sense_breathing_dur);
    // displayText(charBuf, 6, 32, WHITE);
    sense_breathing();
    last_sense_breathing = millis();
  }

  last_sense_altitude_dur = millis()-last_sense_altitude;
  if (last_sense_altitude_dur > RHYTHM_TEMPRES*10) {
    sense_altitude();
    sense_battery();
    last_sense_altitude = millis();
  }

}



void sense_breathing() {
  breathval = analogRead(breath);
  uint8_t samplepos = rhythm_addval(breathval);
  // Serial.println(breathval);
  int val = (breathval-rhythm_get_baseline())*0.4;
  if (val > 15) { val = 15; }
  if (val < -14) { val = -14; }
  tft.drawLine(samplepos, 79, samplepos, 79-val, graph_col);
   if (samplepos+1 < tft.width()) {
     tft.drawLine(samplepos+1, 93, samplepos+1, 64, BLACK);
   } else {
     tft.drawLine(0, 93, 0, 64, BLACK);
   }

  // tft.fillRect(0, 0 , 10*6, 7, BLACK);
  // sprintf(charBuf, "%i", breathval);
  // displayText(charBuf, 0, 0, WHITE);
  //
  // sprintf(charBuf, "%i", rhythm_get_baseline());
  // displayText(charBuf, 6*6, 0, MAGENTA);


  // if (rhythm_oxygen(oxygen_pct)) {
  if (rhythm_oxygen(0.5)) {
    // displayText("*", 100, 0, BLUE);
    digitalWrite(valve, HIGH);
    graph_col = BLUE;
  } else {
    digitalWrite(valve, LOW);
    // tft.fillRect(100, 0 , 105, 7, BLACK);
    graph_col = WHITE;
  }

  // display period
  tft.drawLine(0, 94, tft.width(), 94, BLACK);
  tft.drawLine(0, 94, rhythm_get_period(), 94, RED);

  // display phase
  tft.drawLine(0, 95, tft.width(), 95, BLACK);
  tft.drawLine(0, 95, rhythm_get_phase(), 95, CYAN);

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
    // Serial.print("Temperature: ");
    // Serial.print(temp);
    // Serial.println(" C");

    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    altitude = round(bmp.pressureToAltitude(seaLevelPressure,
                                            event.pressure,
                                            temperature));
    if (altitude < altitude_smoothed + 20000
        && altitude > altitude_smoothed - 1000) {
      altitude_smoothed = 0.8*altitude_smoothed + 0.2*altitude;
      set_oxygen_pct(altitude_smoothed);
    } else {
      // reject
    }

    // tft.fillRect(0, 16 , 5*6, 23, BLACK);
    // sprintf(charBuf, "%im", altitude);
    // displayText(charBuf, 0, 16, WHITE);
  } else {
    // Serial.println("Sensor error");
  }
}


void sense_battery() {
  // AREF used, 0-5.09 -> 0-1023
  // 3.2V low-battery, map this to 0
  // 3.7 to max
  float batvolts = analogRead(battery) * (5.04/1023.0); // map to 0-5.09
  if (batvolts <= 3.2) {
    bat_pct = 0;
  } else if (batvolts >= 4.1) {
    bat_pct = 100;
  } else {
    bat_pct = (batvolts - 3.2) * 111;  // * 100/(4.1-3.2)
  }
  // color
  if (bat_pct > 40) {
    bat_col = GREEN;
  } else if (bat_pct > 15) {
    bat_col = ORANGE;
  } else {
    bat_col = RED;
  }
  // print
  sprintf(charBuf, "%03i", bat_pct);
  tft.fillRect(95, 21, 6*4, 7, BLACK);
  displayText(charBuf, 95, 21, bat_col);
  displayText("%", 95+18, 21, bat_col);
}


void set_oxygen_pct(float alt) {
  // set a oxygen percentaged based on altitude, linearly
  if (alt > oxygen_start_alt) {
    if (alt < OXYGEN_100PCT_ALTITUDE) {
      // map oxygen_start_alt:OXYGEN_100PCT_ALTITUDE -> OXYGEN_MIN_PCT:1.0
      oxygen_pct = (1.0-OXYGEN_MIN_PCT)*(alt-oxygen_start_alt)
                 /(OXYGEN_100PCT_ALTITUDE - oxygen_start_alt) + OXYGEN_MIN_PCT;
    } else {
      oxygen_pct = 1.0; // 100%
    }
  } else {
    oxygen_pct = 0.0; // 0%, below start altitude
  }
}


void displayText(char *text, int16_t x, int16_t y, uint16_t color) {
  tft.setCursor(x,y);
  tft.setTextColor(color);
  tft.print(text);
}



void lcdTestPattern(void)
{
  uint32_t i,j;
  tft.goTo(0, 0);

  for(i=0;i<128;i++)
  {
    for(j=0;j<128;j++)
    {
      if(i<16){
        tft.writeData(RED>>8); tft.writeData(RED);
      }
      else if(i<32) {
        tft.writeData(YELLOW>>8);tft.writeData(YELLOW);
      }
      else if(i<48){tft.writeData(GREEN>>8);tft.writeData(GREEN);}
      else if(i<64){tft.writeData(CYAN>>8);tft.writeData(CYAN);}
      else if(i<80){tft.writeData(BLUE>>8);tft.writeData(BLUE);}
      else if(i<96){tft.writeData(MAGENTA>>8);tft.writeData(MAGENTA);}
      else if(i<112){tft.writeData(BLACK>>8);tft.writeData(BLACK);}
      else {
        tft.writeData(WHITE>>8);
        tft.writeData(WHITE);
       }
    }
  }
}
