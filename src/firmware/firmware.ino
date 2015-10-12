/******************************************************************************
  WaveGlide firmware

  Written by Stefan Hechenberger for Nortd Labs.
  BSD license, all text above must be included in any redistribution

  https://github.com/nortd/WaveGlide
 *****************************************************************************/

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


int breathval = 0;
int breathval_prev = 1023;
bool oxy_on = false;
uint16_t oxy_on_time = 0;
uint16_t oxy_on_dur = 0;
float baro_altitude = 0;
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
  Serial.println("init");
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

  Serial.println(time, DEC);
  delay(500);

  lcdTestPattern();
  delay(500);
  tft.invert(true);
  delay(100);
  tft.invert(false);
  delay(100);
}

void loop() {
  tft.fillScreen(BLACK);
  tft.drawLine(0, tft.height()-12, tft.width(), tft.height()-12, WHITE);
  breathval = analogRead(breath);
  Serial.println(breathval);

  sprintf(charBuf, "val: %i", breathval);
  displayText(charBuf, 0, 0, WHITE);

  // Get a new barometric sensor event
  sensors_event_t event;
  bmp.getEvent(&event);

  // Display the results (barometric pressure is measure in hPa)
  if (event.pressure) {
    Serial.print("Pressure:    ");
    Serial.print(event.pressure);
    Serial.println(" hPa");
    /* Calculating altitude with reasonable accuracy requires pressure    *
     * sea level pressure for your position at the moment the data is     *
     * converted, as well as the ambient temperature in degress           *
     * celcius.  If you don't have these values, a 'generic' value of     *
     * 1013.25 hPa can be used (defined as SENSORS_PRESSURE_SEALEVELHPA   *
     * in sensors.h), but this isn't ideal and will give variable         *
     * results from one day to the next.                                  *
     *                                                                    *
     * You can usually find the current SLP value by looking at weather   *
     * websites or from environmental information centers near any major  *
     * airport.                                                           *
     *                                                                    *
     * For example, for Paris, France you can check the current mean      *
     * pressure and sea level at: http://bit.ly/16Au8ol                   */

    /* First we get the current temperature from the BMP085 */
    float temperature;
    bmp.getTemperature(&temperature);
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    /* Then convert the atmospheric pressure, SLP and temp to altitude    */
    /* Update this next line with the current SLP for better results      */
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    baro_altitude = bmp.pressureToAltitude(seaLevelPressure,
                                           event.pressure,
                                           temperature);

    sprintf(charBuf, "alt: %im", round(baro_altitude));
    displayText(charBuf, 0, 48, WHITE);
  } else {
    // Serial.println("Sensor error");
  }

  // buttonState = digitalRead(button);
  if (breathval < 540 && breathval_prev > 540) {
    displayText(">", 90, 0, BLUE);
    // trigger
    oxy_on = true;
    oxy_on_time = millis();
  }
  oxy_on_dur = millis() - oxy_on_time;

  sprintf(charBuf, "%i", oxy_on_dur);
  displayText(charBuf, 0, 16, WHITE);

  if (oxy_on_dur > 1000) {
    displayText("-", 80, 0, RED);
    oxy_on = false;
  }

  if (oxy_on){
    displayText("*", 100, 0, BLUE);
    digitalWrite(valve, HIGH);
  } else {
    digitalWrite(valve, LOW);
  }

  breathval_prev = breathval;

  delay(200);
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
