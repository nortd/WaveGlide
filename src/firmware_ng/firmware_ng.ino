/*
  firmware.ino - the main firmware file
  Part of the WaveGlide firmware. See: https://github.com/nortd/WaveGlide
  Copyright (c) 2019 Stefan Hechenberger

  The WaveGlide firmware is free software: you can redistribute it and/or
  modify it under the terms of the GNU General Public License as published
  by the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  The firmware is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the  GNU General Public License
  for more details.
*/

// Developed on Arduino 1.8.5

// version
#define VERSION "2.0.0"
#define BLE_ENABLE
#define SPO2_ENABLE


// debug switches
// #define SIMULATE_ALTITUDE 4500

// pulseoxy on serial
#define oxy_rx 0
#define oxy_tx 1

// baro pins
#define sclk 24
#define mosi 23
#define miso 22
#define ble_reset 4
#define ble_irq 7
#define ble_cs 8
#define baro_cs 19

#define breath 18    // differential pressure sensor
#define buzzer1 5    // piezo buzzer pin1
#define buzzer2 6    // piezo buzzer pin2
#define battery 9    // battery voltage
#define led_g 10     // RGB green
#define led_b 11     // RGB blue
#define led_r 12     // RGB red
#define valve 13     // air valve

#define user1 15     // DB9 pin4
#define user2 16     // DB9 pin8
#define button 16    // DB9 pin8 #TODO
#define voltconf 17  // A3, if high activates R1.33 and outputs 12V (instead of 5V) on valve


#include <SPI.h>
#include "Adafruit_BMP280.h"

#ifdef BLE_ENABLE
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
// #include "Adafruit_Bluefruit_nRF51/BluefruitConfig.h"
#define FACTORYRESET_ENABLE 1
#define MINIMUM_FIRMWARE_VERSION "0.6.6"
#define MODE_LED_BEHAVIOUR_DCON "DISABLE" //"DISABLE", "MODE", "BLEUART", "HWUART", "SPI", "MANUAL"
#define MODE_LED_BEHAVIOUR_CON "MODE" //"DISABLE", "MODE", "BLEUART", "HWUART", "SPI", "MANUAL"
Adafruit_BluefruitLE_SPI ble(ble_cs, ble_irq, ble_reset);
// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}
uint16_t last_ble = 0;      // for sample timing
uint16_t last_ble_dur = 0;  // for samble timing

int status_step_ble = 0;
#endif

// status bits, 0-6 (ASCII range)
// (_, signalOK, probeOK, sensorOK, pulseOK, batLOW, spo2LOW, _)
char status_bits = 0;
int status_step_serial = 0;


// pulsoxy
uint16_t last_pulsoxy = 0;      // for sample timing
uint16_t last_pulsoxy_dur = 0;  // for samble timing
int pulsoxy_frame_count = 0;
// frame data
int pulsoxy_byte = 0;
int pulsoxy_syncbit = 0;
int pulseoxy_bcibyte = 0;
int pulsoxy_signalstrength = 0;
// int pulsoxy_signalOK = 1;
// int pulsoxy_probeOK = 1;
// int pulsoxy_pulsebeep = 1;
int pulsoxy_pleth = 0;
// int pulsoxy_bargraph = 0;
// int pulsoxy_sensorOK = 1;
// int pulsoxy_pulseOK = 1;
int pulsoxy_heartratebit7 = 0;
int pulsoxy_heartrate = 0;
int pulsoxy_spo2 = 0;


// #include <Wire.h>
// #include "Adafruit_Sensor.h"

extern "C" {
  #include "rhythm.h"
  #include "pitches.h"
  // #include "bitmaps.c"
}

// breathing sensor
uint16_t last_sense_breathing = 0;      // for sample timing
uint16_t last_sense_breathing_dur = 0;  // for samble timing
int breathval = 0;

// altitude sensor
uint16_t last_sense_altitude = 0;
uint16_t last_sense_altitude_dur = 0;
int temperature = 0;
int altitude = 0;  // pressure alt in m (based on 1013 hPa)
float altitude_smoothed = 0;
int flight_level = 0;  // in feet /100
// barometric sensor via SPI
Adafruit_BMP280 bmp(baro_cs);

// oxygen setting related
#define METERS2FEET 3.28084
#define OXYGEN_100PCT_FL 300  // FL300
#define OXYGEN_100PCT_ALTITUDE 9144  // FL300
int oxygen_start_fls[] = {80, 50}; // FL80, FL50, CAREFUL: length 2 expected
int oxygen_start_alts[] = {2625, 1524}; // FL80, FL50, CAREFUL: length 2 expected
uint8_t alt_setting = 1;  // used as index in above arrays
int oxygen_pct = 0;

// battery voltage monitoring
uint16_t last_sense_battery = 0;
uint16_t last_sense_battery_dur = 0;
float bat_pct = 0.9;
// uint16_t bat_col = WHITE;
uint16_t bat_col = 0;

// interface
volatile bool button_short_handled = true;
volatile uint16_t last_button = 0;
volatile uint16_t last_button_dur = 0;
bool state = LOW;
// adjustment percentages, applied to oxygen_pct
float adj_pcts[] = {0.7, 1.0, 1.3, 100.0};  // CAREFUL: length 4 expected
// uint8_t adj_setting = 0;  // will be 1 after registering interrupt
uint8_t adj_setting = 3;  // will be 1 after registering interrupt
#define SAMPLES_GRAPH_WIDTH 128
uint8_t samplepos = 0;
// char charBuf[50];


void onButton() {
  state = !state;
  last_button_dur = millis()-last_button;
  last_button = millis();
  if (state && last_button_dur > 40) {  // filter bounces
    button_short_handled = false;
  }
}



void setup(void) {
  // analogReference(EXTERNAL);
  pinMode(breath, INPUT);
  // pinMode(button, INPUT_PULLUP);
  pinMode(valve, OUTPUT);
  pinMode(buzzer1, OUTPUT);
  pinMode(buzzer2, OUTPUT);
  pinMode(battery, INPUT);
  pinMode(voltconf, OUTPUT);
  // analogWrite(buzzer, 150);
  pinMode(led_r, OUTPUT);
  pinMode(led_g, OUTPUT);
  pinMode(led_b, OUTPUT);

  analogWrite(led_r, 255);
  analogWrite(led_g, 240);
  analogWrite(led_b, 200);

  digitalWrite(voltconf, HIGH); // conf valve output to 12V

  // check for button held during power up
  bool button_held = false;
  if (!digitalRead(button)) { // pressed
    button_held = true;
  }

  digitalWrite(buzzer1, LOW);
  // tone(buzzer1, 5000, 1000);
  tone(buzzer2, NOTE_E8, 1000);

  // while (!Serial);  // necessary to get Serial
  // delay(1500);       // working right away
  Serial.begin(9600);  // USB
  // Serial.println("init");
  // tft.begin();
  //tft.setRotation(1);  // 1: 90 clockwise, 2: 180, 3: 270
  // tft.fillScreen(BLACK);

  /* Initialise the barometric sensor */
  while(!bmp.begin()) {
    /* There was a problem detecting the BMP085 ... check your connections */
    // Serial.println("No BMP280 detected.");
    tone(buzzer2, NOTE_E3, 500);
    tone(buzzer2, NOTE_E4, 500);
  }
  //
  // /* Default settings from datasheet. */
  // bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
  //                 Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
  //                 Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
  //                 Adafruit_BMP280::FILTER_X16,      /* Filtering. */
  //                 Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */


  // set alt mode
  if (button_held) {
    // If button was helled during power-up set to next alt mode.
    // Specifically this means oxygenation starts at a higher alt.
    alt_setting++;
    if (alt_setting >= 2) { alt_setting = 0; }
    // sprintf(charBuf, "FL%03i", oxygen_start_fls[alt_setting]);
    // // draw
    // if (alt_setting == 0) {
    //   tft.fillRect(63, 4, 32, 7, BLACK);
    //   displayText(charBuf, 64, 4, OXYBLUE);
    //   tft.fillRect(124, 8, 4, 48, OXYBLUE);
    // } else if (alt_setting == 1) {
    //   tft.fillRect(63, 4, 32, 7, OXYBLUE);
    //   displayText(charBuf, 64, 4, BLACK);
    //   tft.fillRect(124, 8, 4, 48, BLACK);
    //   tft.fillRect(124, 8, 4, map(oxygen_start_fls[alt_setting], 0, OXYGEN_100PCT_FL, 48, 0), OXYBLUE);
    // }
  }

  // enable button interrupt
  // attachInterrupt(digitalPinToInterrupt(button), onButton, CHANGE);

  #ifdef BLE_ENABLE
  // Serial.print(F("Initialising the Bluefruit LE module: "));
  // if ( !ble.begin(VERBOSE_MODE) ) {
  if ( !ble.begin() ) {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  // Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE ) {
    // Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
    // Serial.println( F("OK!") );
  }

  ble.echo(false);
  // ble.info();
  ble.verbose(false);
  #endif

  tone(buzzer2, NOTE_E7, 1000);

  #ifdef SPO2_ENABLE
  Serial1.begin(4800); // RX/TX pins
  #endif


  tone(buzzer2, NOTE_E6, 1000);
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

  #ifdef BLE_ENABLE
  last_ble_dur = millis()-last_ble;
  if (last_ble_dur > 300) {
    handle_ble();
    last_ble = millis();
  }
  #endif

  #ifdef SPO2_ENABLE
  last_pulsoxy_dur = millis()-last_pulsoxy;
  if (last_pulsoxy_dur > 1) {
    handle_pulsoxy();
    last_pulsoxy = millis();
  }
  #else
  last_pulsoxy_dur = millis()-last_pulsoxy;
  if (last_pulsoxy_dur > 1000) {
    send_status_serial();
    last_pulsoxy = millis();
  }
  #endif

  // handle short button press
  // if (!button_short_handled) {
  //   handle_short_button();
  //   button_short_handled = true;
  // }

  // #ifdef SPO2_ENABLE
  // // forward YS2000 for debugging
  // if (Serial1.available()) {     // If anything comes in Serial1 (pins 0 & 1)
  //   Serial.write(Serial1.read());   // read it and send it out Serial (USB)
  // }
  // #endif
}



void sense_breathing() {
  breathval = analogRead(breath);
  rhythm_addval(breathval);
  // Serial.println(breathval);
  int val = (breathval-rhythm_get_baseline())*0.4;
  if (val > 12) { val = 12; }
  if (val < -12) { val = -12; }
  // if (!baseline_set()) { graph_col = DARKBLUE; }
  if (rhythm_oxygen(oxygen_pct)) {
    digitalWrite(valve, HIGH);
  } else {
    digitalWrite(valve, LOW);
  }
  // display
  analogWrite(led_b, breathval*10);
}



void sense_altitude() {
    temperature = round(bmp.readTemperature());
    altitude = bmp.readAltitude(1013.25);

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
    // flight level, in feet, /100, rounded to nearest 500
    // flight_level = round((altitude_smoothed * METERS2FEET)/500) * 5;
    // flight level, do not round to nearest 500
    flight_level = round((altitude_smoothed * METERS2FEET)/100);
    flight_level = constrain(flight_level, 0, OXYGEN_100PCT_FL);
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
}



void sense_battery() {
  // 0-1023 -> 0-3.3
  // Feather M0, has default AREF of 3.3 and a voltage divider.
  // Lipos are maxed out at 4.2, then discharge mostly around 3.7, cutoff at 3.2V
  float batvolts = analogRead(battery) * 0.0064453125;  // 2*(3.3/1024.0);
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
}



void handle_short_button() {
  adj_setting++;
  if (adj_setting >= 4) { adj_setting = 0; }
  // update oxygen_pct
  set_oxygen_pct(altitude_smoothed);
}


#ifdef BLE_ENABLE
void handle_ble() {
  // Handle BLE connection Events
  if (ble.isConnected()) {
    // LED Activity command is only supported from 0.6.6
    if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
      // Change Mode LED Activity
      // Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR_CON));
      ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR_CON);
    }
    ble.setMode(BLUEFRUIT_MODE_DATA);
  } else {
    // LED Activity command is only supported from 0.6.6
    if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) ) {
      // Change Mode LED Activity
      // Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR_DCON));
      ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR_DCON);
    }
  }
  // Handle BLE tx
  // ble.print(inputs);
  // ble.print("+\n");

  send_status_ble();
  // ble.print(oxygen_pct);
  // ble.print("% ");
  // ble.print(altitude);
  // ble.print("m ");
  // ble.print(round(bat_pct*100));
  // ble.print("bat ");
  // ble.print("\n");

  // Handle BLE rx
  // while (ble.available()) {
  //   int c = ble.read();
  //
  //   Serial.print((char)c);
  //
  //   // Hex output too, helps w/debugging!
  //   // Serial.print(" [0x");
  //   // if (c <= 0xF) Serial.print(F("0"));
  //   // Serial.print(c, HEX);
  //   // Serial.print("] ");
  // }
}
#endif


#ifdef SPO2_ENABLE
void handle_pulsoxy(){
  // Serial.print("handle_pulsoxy\n");
  if (Serial1.available()) {
    pulsoxy_byte = Serial1.read();
    pulsoxy_syncbit = (pulsoxy_byte&1<<7)>>7;
    if (pulsoxy_syncbit == 1){
      pulseoxy_bcibyte = 0;
    }
    // parse bytes
    // status_bits ... (_, signalOK, probeOK, sensorOK, pulseOK, batLOW, spo2LOW, _)
    if (pulseoxy_bcibyte == 0) {
      pulsoxy_signalstrength = pulsoxy_byte&7;  // 3 bits, 0-7
      // pulsoxy_signalOK = (pulsoxy_byte&1<<4)>>4; // 0=OK
      status_bits = (status_bits&0b10111111) | ((~pulsoxy_byte&0b00010000)<<2); // bit 4 into bit 6, invert
      // pulsoxy_probeOK = (pulsoxy_byte&1<<5)>>5; // 0=CONNECTED
      status_bits = (status_bits&0b11011111) | (~pulsoxy_byte&0b00100000); // bit 5 into bit 5, invert
      // pulsoxy_pulsebeep = (pulsoxy_byte&1<<6)>>6;
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 1){
      pulsoxy_pleth = (pulsoxy_byte&127); // first 7 bits
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 2){
      // pulsoxy_bargraph = pulsoxy_byte&7;
      // pulsoxy_sensorOK = (pulsoxy_byte&1<<4)>>4; // 0=OK
      status_bits = (status_bits&0b11101111) | (~pulsoxy_byte&0b00010000); // bit 4 into bit 4, invert
      // pulsoxy_pulseOK = (pulsoxy_byte&1<<5)>>5;  // 0=OK
      status_bits = (status_bits&0b11110111) | ((~pulsoxy_byte&0b00100000)>>2); // bit 5 into bit 3, invert
      pulsoxy_heartratebit7 = (pulsoxy_byte&1<<6)<<1;
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 3){
      pulsoxy_heartrate = pulsoxy_heartratebit7|(pulsoxy_byte&127); // bits 0-6 plus the 7th
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 4){
      pulsoxy_spo2 = (pulsoxy_byte&127); // first 7 bits, 0-100%
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 5){
      // not used
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 6){
      // not used
      pulseoxy_bcibyte = -1;
      pulsoxy_frame_count += 1;
      if (pulsoxy_frame_count > 10) {
        #ifndef BLE_ENABLE
        send_status_serial();
        #endif
        pulsoxy_frame_count = 0;
      }
    } else {
      // out of sync
    }
  }
}
#endif


void send_status_serial(){
  // send via usbserial
  // for Arduino serial plotter
  if (status_step_serial == 0) {
    Serial.print(oxygen_pct);
    Serial.print("\t");
    status_step_serial += 1;
  } else if (status_step_serial == 1) {
    Serial.print(flight_level);
    Serial.print("\t");
    status_step_serial += 1;
  } else if (status_step_serial == 2) {
    Serial.print(round(bat_pct*100));
    Serial.print("\t");
    status_step_serial += 1;
  } else if (status_step_serial == 3) {
    Serial.print(pulsoxy_signalstrength);
    Serial.print("\t");
    status_step_serial += 1;
  } else if (status_step_serial == 4) {
    Serial.print(pulsoxy_pleth);
    Serial.print("\t");
    status_step_serial += 1;
  } else if (status_step_serial == 5) {
    Serial.print(pulsoxy_heartrate);
    Serial.print("\t");
    status_step_serial += 1;
  } else if (status_step_serial == 6) {
    Serial.print(pulsoxy_spo2);
    Serial.print("\t");
    status_step_serial += 1;
  } else if (status_step_serial == 7) {
    // explode status bits, map to 0 and 255 for visible plotting
    // status_bits ... (_, signalOK, probeOK, sensorOK, pulseOK, batLOW, spo2LOW, _)
    //signalOK
    Serial.print(map((status_bits&0b01000000)>>6, 0, 1, 0, 255));
    Serial.print("\t");
    //probeOK
    Serial.print(map((status_bits&0b00100000)>>5, 0, 1, 0, 255));
    Serial.print("\t");
    //sensorOK
    Serial.print(map((status_bits&0b00010000)>>4, 0, 1, 0, 255));
    Serial.print("\t");
    //pulseOK
    Serial.print(map((status_bits&0b00001000)>>3, 0, 1, 0, 255));
    Serial.println("\t");  // println since last
    status_step_serial = 0;  // start over
  }
}

#ifdef BLE_ENABLE
void send_status_ble(){
  if (ble.isConnected()) {
    // send via BLE
    if (status_step_ble == 0) {
      ble.print(oxygen_pct);
      ble.print("\t");
      status_step_ble += 1;
    } else if (status_step_ble == 1) {
      ble.print(flight_level);
      ble.print("\t");
      status_step_ble += 1;
    } else if (status_step_ble == 2) {
      ble.print(round(bat_pct*100));
      ble.print("\t");
      status_step_ble += 1;
    } else if (status_step_ble == 3) {
      ble.print(pulsoxy_signalstrength);
      ble.print("\t");
      status_step_ble += 1;
    } else if (status_step_ble == 4) {
      ble.print(pulsoxy_pleth);
      ble.print("\t");
      status_step_ble += 1;
    } else if (status_step_ble == 5) {
      ble.print(pulsoxy_heartrate);
      ble.print("\t");
      status_step_ble += 1;
    } else if (status_step_ble == 6) {
      ble.print(pulsoxy_spo2);
      ble.print("\t");
      status_step_ble += 1;
    } else if (status_step_ble == 7) {
      ble.println(status_bits);
      status_step_ble = 0;  // start over
    }
  }
}
#endif
