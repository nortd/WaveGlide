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
#define button 5
#define power_ssr 6
#define buzzer 3

// air valve
#define valve A0

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

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

uint16_t last_control_valve = 0;
uint16_t last_control_valve_dur = 0;
uint16_t oxy_on_time = 0;
uint16_t oxy_on_dur = 0;

int buttonState = 0;
char charBuf[50];



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
  pinMode(breath, INPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(power_ssr, OUTPUT);
  digitalWrite(power_ssr, LOW);  // maintain power to board
  pinMode(valve, OUTPUT);
  pinMode(buzzer, OUTPUT);
  // analogWrite(buzzer, 150);

  Serial.begin(9600);
  // Serial.println("init");
  tft.begin();
  //tft.setRotation(1);  // 1: 90 clockwise, 2: 180, 3: 270

  /* Initialise the barometric sensor */
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
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

  tft.fillScreen(BLACK);
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
  if (last_sense_altitude_dur > 1000) {
    sense_altitude();
    last_sense_altitude = millis();
  }


  last_control_valve_dur = millis()-last_control_valve;
  if (last_control_valve_dur > 50) {
    control_valve();
    last_control_valve = millis();
  }
}



void sense_breathing() {
  breathval = analogRead(breath);
  uint8_t samplepos = rhythm_addval(breathval);
  Serial.println(breathval);
  tft.drawLine(samplepos, tft.height()-10,
               samplepos, tft.height()-10-(breathval-rhythm_get_baseline())*0.5,
               graph_col);
   if (samplepos+1 < tft.width()) {
     tft.drawLine(samplepos+1, tft.height(),
                  samplepos+1, tft.height()-40,
                  BLACK);
   } else {
     tft.drawLine(0, tft.height(),
                  0, tft.height()-40,
                  BLACK);
   }

  tft.fillRect(0, 0 , 10*6, 7, BLACK);
  sprintf(charBuf, "%i", breathval);
  displayText(charBuf, 0, 0, WHITE);

  sprintf(charBuf, "%i", rhythm_get_baseline());
  displayText(charBuf, 6*6, 0, MAGENTA);


  if (rhythm_oxygen(1.0)) {
    // displayText("*", 100, 0, BLUE);
    digitalWrite(valve, HIGH);
    graph_col = BLUE;
  } else {
    digitalWrite(valve, LOW);
    // tft.fillRect(100, 0 , 105, 7, BLACK);
    graph_col = WHITE;
  }

  // display period
  tft.drawLine(0, tft.height()-41, tft.width(), tft.height()-41, BLACK);
  tft.drawLine(0, tft.height()-41, rhythm_get_period(), tft.height()-41, RED);

  // display phase
  tft.drawLine(0, tft.height()-42, tft.width(), tft.height()-42, BLACK);
  tft.drawLine(0, tft.height()-42, rhythm_get_phase(), tft.height()-42, CYAN);

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

    tft.fillRect(0, 16 , 5*6, 23, BLACK);
    sprintf(charBuf, "%im", altitude);
    displayText(charBuf, 0, 16, WHITE);
  } else {
    // Serial.println("Sensor error");
  }
}


void control_valve() {

  // if (rhythm_oxygen(0.2)) {
  //   // displayText("*", 100, 0, BLUE);
  //   digitalWrite(valve, HIGH);
  //   graph_col = BLUE;
  // } else {
  //   digitalWrite(valve, LOW);
  //   // tft.fillRect(100, 0 , 105, 7, BLACK);
  //   graph_col = WHITE;
  // }
  //
  // // display period
  // tft.drawLine(0, tft.height()-41, tft.width(), tft.height()-41, BLACK);
  // tft.drawLine(0, tft.height()-41, rhythm_get_period(), tft.height()-41, RED);
  //
  // // display phase
  // tft.drawLine(0, tft.height()-42, tft.width(), tft.height()-42, BLACK);
  // tft.drawLine(0, tft.height()-42, rhythm_get_phase(), tft.height()-42, CYAN);

  //
  // // buttonState = digitalRead(button);
  // if (breathval < 510 && breathval_prev > 510) {
  //   displayText(">", 90, 0, BLUE);
  //   // trigger
  //   oxy_on = true;
  //   oxy_on_time = millis();
  // } else {
  //   tft.fillRect(90, 0 , 95, 7, BLACK);
  // }
  // oxy_on_dur = millis() - oxy_on_time;
  //
  // // sprintf(charBuf, "%i", oxy_on_dur);
  // // tft.fillRect(0, 16, 128, 23, BLACK);
  // // displayText(charBuf, 0, 16, WHITE);
  //
  // if (oxy_on_dur > 1000) {
  //   displayText("-", 80, 0, RED);
  //   oxy_on = false;
  // } else {
  //   tft.fillRect(80, 0 , 85, 7, BLACK);
  // }
  //
  // if (oxy_on){
  //   displayText("*", 100, 0, BLUE);
  //   digitalWrite(valve, HIGH);
  //   graph_col = BLUE;
  // } else {
  //   digitalWrite(valve, LOW);
  //   tft.fillRect(100, 0 , 105, 7, BLACK);
  //   graph_col = WHITE;
  // }
  //
  // breathval_prev = breathval;
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
