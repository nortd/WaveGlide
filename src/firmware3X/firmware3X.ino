/*
  firmware.ino - the main firmware file
  Part of the WaveGlide firmware. See: https://github.com/nortd/WaveGlide
  Copyright (c) 2020 Stefan Hechenberger

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
#define VERSION "2.0.1"
#define BLE_ENABLE
#define SPO2_ENABLE
#define BARO_ENABLE
#define SERIAL_DEBUG_ENABLE
// #define SIMULATE_ALTITUDE 4500

#define STATUS_SPO2_CAUTION 93
#define STATUS_SPO2_LOW 88
#define STATUS_BAT_CAUTION 0.5
#define STATUS_BAT_LOW 0.3
#define STATUS_BREATH_LOW 15
#define STATUS_BREATH_FAST 1000
#define PULSOXY_MIN_SIGNAL 2

#define C_BLUE 0x0033ff
#define C_GREEN 0x00ff11
#define C_YELLOW 0xffff00
#define C_ORANGE 0xff7700
#define C_RED 0xff0000
#define C_PINK 0xff00ff
#define C_BLACK 0x000000
#define C_WHITE 0xffffff

#define LOWKEY_BRIGHTNESS 15
#define NOTICE_BRIGHTNESS 50
#define CAUTION_BRIGHTNESS 80
#define WARN_BRIGHTNESS 100

// pulseoxy on default serial
#define spo2_rx 0
#define spo2_tx 1

// BLE and BARO SPI on default pins
#define sclk 24
#define mosi 23
#define miso 22
#define ble_reset 4
#define ble_irq 7
#define ble_cs 8
#define baro_cs 19

#define breath 18    // differential pressure sensor
#ifdef VERSION31
  #define buzzer1 5    // piezo buzzer pin1
  #define buzzer2 6    // piezo buzzer pin2
#else
  #define buzzer1 6    // not used
  #define buzzer2 5    // piezo buzzer pin
#endif
#define battery 9    // battery voltage
#define led_g 10     // RGB green
#define led_b 11     // RGB blue
#define led_r 12     // RGB red
#define valve 13     // air valve

#define button1 15   // DB9 pin4, pressed by shorting it to DB9 pin1
#define button2 16   // DB9 pin8, pressed by shorting it to DB9 pin1
#define voltconf 17  // A3, if high activates R1.33 and outputs 12V (instead of 5V) on valve


#include <SPI.h>
#include "Adafruit_BMP280.h"

#ifdef BLE_ENABLE
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
// #include "Adafruit_Bluefruit_nRF51/BluefruitConfig.h"
#define MINIMUM_FIRMWARE_VERSION "0.6.6"
Adafruit_BluefruitLE_SPI ble(ble_cs, ble_irq, ble_reset);
uint16_t last_ble = 0;      // for sample timing
uint16_t last_ble_dur = 0;  // for samble timing
uint8_t status_step_ble = 0;
#endif
bool ble_OK = false;

uint16_t last_serial = 0;
uint16_t last_serial_dur = 0;
uint8_t status_step_serial = 0;

// pulsoxy
bool pulsoxy_mode = false;
uint16_t last_pulsoxy = 0;      // for sample timing
uint16_t last_pulsoxy_dur = 0;  // for samble timing
uint8_t pulsoxy_frame_count = 0;
// frame data
uint8_t pulsoxy_byte = 0;
uint8_t pulsoxy_syncbit = 0;
uint8_t pulseoxy_bcibyte = 0;
uint8_t pulsoxy_signalstrength = 0;
// int pulsoxy_signalOK = 1;
// int pulsoxy_probeOK = 1;
// int pulsoxy_pulsebeep = 1;
uint8_t pulsoxy_pleth = 0;
// int pulsoxy_bargraph = 0;
// int pulsoxy_sensorOK = 1;
// int pulsoxy_pulseOK = 1;
uint8_t pulsoxy_heartratebit7 = 0;
uint8_t pulsoxy_heartrate = 0;
uint8_t pulsoxy_spo2 = 0;
// status bits, 0-6 (ASCII range)
// (_, signalOK, probeOK, sensorOK, pulseOK, _, _, pulsoxyOK)
// // signalOK ... any signal from probe
// // probeOK ... finger probe connected
// // sensorOK ... finger in probe
// // pulseOK ... reading a hr/spo2
// // pulsoxyOK ... serial to module connected
char pulsoxy_status_bits = 0;
// frame fields to push them to serial/ble in a consistent state
uint8_t frame_pulsoxy_signalstrength = 0;
uint8_t frame_pulsoxy_heartrate = 0;
uint8_t frame_pulsoxy_spo2 = 0;
char frame_pulsoxy_status_bits = 0;


extern "C" {
  #include "rhythm.h"
  #include "pitches.h"
}

// feedback
uint16_t last_feedback = 0;
uint16_t last_feedback_dur = 0;
uint16_t last_inhale = 0;
uint16_t last_inhale_dur = 0;
uint16_t last_exhale = 0;
uint16_t last_exhale_dur = 0;
uint16_t last_idle = 0;
uint16_t last_idle_dur = 0;

// breathing sensor
uint16_t last_sense_breathing = 0;      // for sample timing
uint16_t last_sense_breathing_dur = 0;  // for samble timing
int breathval = 0;
bool valve_on = false;

// altitude sensor
uint16_t last_sense_altitude = 0;
uint16_t last_sense_altitude_dur = 0;
int temperature = 0;
int altitude = 0;  // pressure alt in m (based on 1013 hPa)
float altitude_smoothed = 0;
int flight_level = 0;  // in feet /100
#ifdef BARO_ENABLE
// barometric sensor via SPI
Adafruit_BMP280 bmp(baro_cs);
#endif

// oxygen setting related
#define METERS2FEET 3.28084
#define OXYGEN_100PCT_ALTITUDE 9144  // FL300
int oxygen_start_alts[] = {2625, 1524}; // FL80, FL50, CAREFUL: length 2 expected
uint8_t alt_setting = 1;  // used as index in above arrays
int oxygen_pct = 0; // 0-100
int oxygen_total_ms = 0;  // FYI: int is 4 bits and overflows at 2147483647 or 596h
uint16_t last_oxygen_on = 0;

// battery voltage monitoring
void sense_battery(bool noSmooth=false);
uint16_t last_sense_battery = 0;
uint16_t last_sense_battery_dur = 0;
float bat_pct = 0.9;

// general status bits
// (_, _, bleFAIL, baroFAIL, breathFAST, breathLOW, batLOW, spo2LOW)
// // bleFAIL ... SPI to module failure
// // baroFAIL ... SPI to module failure
// // breathFAST ... breathing rhythm is very fast
// // breathLOW ... breathing amplitude is very low
// // batLOW ... battery is running low
// // spo2LOW ... blood oxygen is very low
char status_bits = 0;

// buttons
volatile bool button1_short_handled = true;
volatile uint16_t last_button1 = 0;
volatile uint16_t last_button1_dur = 0;
bool button1_state = LOW;
volatile bool button2_short_handled = true;
volatile uint16_t last_button2 = 0;
volatile uint16_t last_button2_dur = 0;
bool button2_state = LOW;

// interface
// adjustment percentages, applied to oxygen_pct
float adj_pcts = 1.0;

int rgb_r = 0;
int rgb_g = 0;
int rgb_b = 0;


void onButton1() {
  button1_state = !button1_state;
  last_button1_dur = millis()-last_button1;
  last_button1 = millis();
  if (button1_state && last_button1_dur > 40) {  // filter bounces
    button1_short_handled = false;
  }
}

void onButton2() {
  button2_state = !button2_state;
  last_button2_dur = millis()-last_button2;
  last_button2 = millis();
  if (button2_state && last_button2_dur > 40) {  // filter bounces
    button2_short_handled = false;
  }
}

void set_led_off() {
  // Using digitalWrite because with analogWrite(255)
  // the LED still has a tiny bit of visible illumination.
  pinMode(led_r, OUTPUT);
  pinMode(led_g, OUTPUT);
  pinMode(led_b, OUTPUT);
  digitalWrite(led_r, HIGH);
  digitalWrite(led_g, HIGH);
  digitalWrite(led_b, HIGH);
}

void set_led_color(int hexcolor, int lum_pct=100) {
  rgb_r = (hexcolor>>16)&0xFF;
  rgb_g = (hexcolor>>8)&0xFF;
  rgb_b = hexcolor&0xFF;
  lum_pct = constrain(lum_pct, 0, 100);
  if (lum_pct == 0) {
    set_led_off();
  } else {
    analogWrite(led_r, 255-(rgb_r*lum_pct)/100);
    analogWrite(led_g, 255-(rgb_g*lum_pct)/100);
    analogWrite(led_b, 255-(rgb_b*lum_pct)/100);
  }
}

void set_led_color_lerp(int c1, int c2, int t) {
  rgb_r =  (t*(((c2>>16)&0xFF)-((c1>>16)&0xFF)))/100 + ((c1>>16)&0xFF);
  rgb_g =  (t*(((c2>>8)&0xFF)-((c1>>8)&0xFF)))/100 + ((c1>>8)&0xFF);
  rgb_b =  (t*((c2&0xFF)-(c1&0xFF)))/100 + (c1&0xFF);
  analogWrite(led_r, 255-rgb_r);
  analogWrite(led_g, 255-rgb_g);
  analogWrite(led_b, 255-rgb_b);
}


void setup(void) {
  // analogReference(EXTERNAL);
  pinMode(breath, INPUT);
  pinMode(buzzer1, OUTPUT);
  pinMode(buzzer2, OUTPUT);
  pinMode(battery, INPUT);
  pinMode(led_r, OUTPUT);
  pinMode(led_g, OUTPUT);
  pinMode(led_b, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(voltconf, OUTPUT);

  // init status rgb led
  for (int t=0; t<100; t++) {
    set_led_color_lerp(C_WHITE, C_GREEN, t);
    delay(10);
  }
  delay(600);
  set_led_off();

  // set valve voltage to 12V
  digitalWrite(voltconf, HIGH);

  // check for button held during power up
  bool button1_held = false;
  if (!digitalRead(button1)) { // pressed
    button1_held = true;
  }
  bool button2_held = false;
  if (!digitalRead(button2)) { // pressed
    button2_held = true;
  }

  // init buzzer
  digitalWrite(buzzer1, LOW);

  #ifdef SERIAL_DEBUG_ENABLE
  // while (!Serial);  // necessary to get Serial
  // delay(1000);      // working right away
  Serial.begin(9600);  // USB
  #endif

  #ifdef BARO_ENABLE
  while(!bmp.begin()) {
    delay(100);
  }
  // set baroOK bit to TRUE
  // (_, _, bleFAIL, baroFAIL, breathFAST, breathLOW, batLOW, spo2LOW)
  status_bits = status_bits|0b00010000;
  #endif

  // set alt mode
  if (button1_held) {
    // DISABLED for now
    // If button was helled during power-up set to next alt mode.
    // Specifically this means oxygenation starts at a higher alt.
    // alt_setting++;
    // if (alt_setting >= 2) { alt_setting = 0; }
  }

  if (button2_held) {
    // DISABLED
    // TODO: RESET
  }

  // enable button interrupt
  // attachInterrupt(digitalPinToInterrupt(button1), onButton1, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(button2), onButton2, CHANGE);


  #ifdef BLE_ENABLE
  // if ( !ble.begin(VERBOSE_MODE) ) {
  if (ble.begin()) {
    // set bleOK bit to TRUE
    // (_, _, bleFAIL, baroFAIL, breathFAST, breathLOW, batLOW, spo2LOW)
    status_bits = status_bits|0b00100000;
  }
  if (!ble.factoryReset()) {
    // set bleOK bit to FALSE
    status_bits = status_bits&0b11011111;
  }
  ble.echo(false);
  // ble.info();
  ble.verbose(false);
  #endif


  #ifdef SPO2_ENABLE
  Serial1.begin(4800);  // RX/TX pins
  // set pulsoxyOK bit to TRUE
  // (_, signalOK, probeOK, sensorOK, pulseOK, _, _, pulsoxyOK)
  pulsoxy_status_bits = pulsoxy_status_bits|0b00000001;
  #endif

  // battery voltage feedback
  sense_battery(true);
  int bat_stat_beeps = 3;
  int bat_stat_color = C_GREEN;
  if (bat_pct < 0.5) {
    bat_stat_beeps = 1;
    bat_stat_color = C_RED;
  } else if(bat_pct < 0.8) {
    bat_stat_beeps = 2;
    bat_stat_color = C_ORANGE;
  }
  for (int i=0; i<bat_stat_beeps; i++) {
    delay(300);
    tone(buzzer2, NOTE_E6);
    set_led_color(bat_stat_color, NOTICE_BRIGHTNESS);
    delay(300);
    noTone(buzzer2);
    set_led_off();
  }
}



void loop() {
  last_feedback_dur = millis()-last_feedback;
  if (last_feedback_dur > 5) {
    handle_feedback();
    last_feedback = millis();
  }

  last_sense_breathing_dur = millis()-last_sense_breathing;
  if (last_sense_breathing_dur > RHYTHM_TEMPRES) {
    sense_breathing();
    last_sense_breathing = millis();
  }

  last_sense_battery_dur = millis()-last_sense_battery;
  if (last_sense_battery_dur > RHYTHM_TEMPRES*30) {
    sense_battery();
    last_sense_battery = millis();
  }

  #ifdef BARO_ENABLE
  last_sense_altitude_dur = millis()-last_sense_altitude;
  if (last_sense_altitude_dur > RHYTHM_TEMPRES*20) {
    sense_altitude();
    last_sense_altitude = millis();
  }
  #endif

  #ifdef SPO2_ENABLE
  last_pulsoxy_dur = millis()-last_pulsoxy;
  if (last_pulsoxy_dur > 1) {
    handle_pulsoxy();
    last_pulsoxy = millis();
  }
  #endif

  #ifdef BLE_ENABLE
  last_ble_dur = millis()-last_ble;
  if (last_ble_dur > 300) {
    handle_ble();
    last_ble = millis();
  }
  #endif

  #ifdef SERIAL_DEBUG_ENABLE
  last_serial_dur = millis()-last_serial;
  if (last_serial_dur > 1000) {
    send_status_serial();
    last_serial = millis();
  }
  #endif

  // handle short button presses
  if (!button1_short_handled) {
    handle_short_button1();
    button1_short_handled = true;
  }
  if (!button2_short_handled) {
    handle_short_button2();
    button2_short_handled = true;
  }

  // // forward YS2000 for debugging
  // if (Serial1.available()) {     // If anything comes in Serial1 (pins 0 & 1)
  //   Serial.write(Serial1.read());   // read it and send it out Serial (USB)
  // }
}



void sense_breathing() {
  breathval = analogRead(breath);
  rhythm_addval(breathval);
  if (rhythm_oxygen(oxygen_pct)) {
    if (!valve_on) {
      digitalWrite(valve, HIGH);
      last_oxygen_on = millis();
      valve_on = true;
    }
  } else {
    if (valve_on) {
      digitalWrite(valve, LOW);
      oxygen_total_ms += (millis()-last_oxygen_on);
      valve_on = false;
    }
  }
}


void handle_feedback() {
  if (baseline_set()) {
    if (rhythm_get_phase() == PHASE_INHALE) {  // inhale
      // green ... baro mode OK
      // blue ... spo2 mode OK
      // orange ... spo2 caution
      // red ... spo2 warning
      if (last_inhale == 0) {  // inhale starts
        last_inhale = millis();
        last_exhale = 0;  // end exhale
        last_idle = 0;  // end idle
      } else {  // inhale ongoing
        last_inhale_dur = millis()-last_inhale;
        if (pulsoxy_mode) {
          if (pulsoxy_spo2 <= STATUS_SPO2_LOW) {
            set_led_color(C_RED, WARN_BRIGHTNESS);
          } else if (pulsoxy_spo2 <= STATUS_SPO2_CAUTION) {
            set_led_color(C_ORANGE, CAUTION_BRIGHTNESS);
          } else {
            set_led_color(C_BLUE, NOTICE_BRIGHTNESS);
          }
        } else {
          set_led_color(C_GREEN, NOTICE_BRIGHTNESS);
        }
      }
    } else if (rhythm_get_phase() == PHASE_EXHALE) {  // exhale
      // green-green-green ... OK, BLE disconnected
      // green-green-blue ... OK, BLE connected
      // orange-orange ... bat caution
      // red ... bat warning
      if (last_exhale == 0) {  // exhale starts
        last_exhale = millis();
        last_inhale = 0;  // end inhale
        last_idle = 0;  // end idle
        set_led_off();
      } else {  // exhale ongoing
        last_exhale_dur = millis()-last_exhale;
        handle_feedback_status_triple(last_exhale_dur, 500);
      }
      last_idle = 0;  // end idle
    } else if (rhythm_get_phase() == PHASE_IDLE) {  // breath idle
      if (last_idle == 0) {  // idle starts
        last_idle = millis();
        last_inhale = 0;  // end inhale
        last_exhale = 0;  // end exhale
      } else {  // idle ongoing
        last_idle_dur = millis()-last_idle;
        if (last_idle_dur > 10000) {
          last_idle = 0;
        } else {
          handle_feedback_status_triple(last_idle_dur, 500);
        }
      }
    }
  }
}


void handle_feedback_status_triple(uint16_t last_dur, uint16_t start_delay) {
  if (last_dur > start_delay) {
    // three blinks
    // blink1on-link1off-blink2on-blink2off-blink3on-blink3off
    //        50     150       50       150      50
    //     0     50      200       250      400     450
    if (last_dur > start_delay+450) {  // blink3off
      set_led_off();
    } else if (last_dur > start_delay+400) {  // blink3on
      if (ble.isConnected()) {
        set_led_color(C_BLUE, LOWKEY_BRIGHTNESS);
      } else {
        set_led_color(C_GREEN, LOWKEY_BRIGHTNESS);
      }
    } else if (last_dur > start_delay+250) {  // blink2off
      set_led_off();
    } else if (last_dur > start_delay+200) {  // blink2on
      if (bat_pct <= STATUS_BAT_CAUTION) {
        set_led_color(C_ORANGE, LOWKEY_BRIGHTNESS);
      } else {
        set_led_color(C_GREEN, LOWKEY_BRIGHTNESS);
      }
    } else if (last_dur > start_delay+50) {  // blink1off
      set_led_off();
    } else {  // blink1on
      if (bat_pct <= STATUS_BAT_LOW) {
        set_led_color(C_RED, LOWKEY_BRIGHTNESS);
      } else if (bat_pct <= STATUS_BAT_CAUTION) {
        set_led_color(C_ORANGE, LOWKEY_BRIGHTNESS);
      } else {
        set_led_color(C_GREEN, LOWKEY_BRIGHTNESS);
      }
    }
  }
}

#ifdef BARO_ENABLE
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
    // flight level, in feet, /100, rounded to nearest 500 (definition)
    // flight_level = round((altitude_smoothed * METERS2FEET)/500) * 5;
    // flight level, do not round to nearest 500
    flight_level = round((altitude_smoothed * METERS2FEET)/100);
}
#endif



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
  if (pulsoxy_mode) {
    if (pulsoxy_spo2 <= 95) {
      adj_pcts = 0.9*adj_pcts + 0.1*(0.01*map(pulsoxy_spo2, 85, 95, 200, 100));
    } else if (pulsoxy_spo2 > 95) {
      adj_pcts = 0.9*adj_pcts + 0.1*(0.01*map(pulsoxy_spo2, 96, 100, 100, 10));
    }
    oxygen_pct = oxygen_pct * adj_pcts;
  } else {
    oxygen_pct = 100;
  }
  oxygen_pct = constrain(oxygen_pct, 0, 100);

}



void sense_battery(bool noSmooth) {
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
  if (noSmooth) {
    bat_pct = newbat_pct;
  } else{
    bat_pct = 0.8*bat_pct + 0.2*newbat_pct;
  }
  // set battery low bit
  if (bat_pct < STATUS_BAT_LOW) {
    // (_, _, bleFAIL, baroFAIL, breathFAST, breathLOW, batLOW, spo2LOW)
    status_bits = status_bits|0b00000010;
  } else {
    status_bits = status_bits&0b11111101;
  }
}



void handle_short_button1() {
  // NOT USED
}

void handle_short_button2() {
  // NOT USED
}


#ifdef BLE_ENABLE
void handle_ble() {
  // Handle BLE connection Events
  if (ble.isConnected()) {
    // LED Activity command is only supported from 0.6.6
    if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
      // Change Mode LED Activity
      // "DISABLE", "MODE", "BLEUART", "HWUART", "SPI", "MANUAL"
      ble.sendCommandCheckOK("AT+HWModeLED=MODE");
    }
    ble.setMode(BLUEFRUIT_MODE_DATA);
  } else {
    // LED Activity command is only supported from 0.6.6
    if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) ) {
      // Change Mode LED Activity
      // "DISABLE", "MODE", "BLEUART", "HWUART", "SPI", "MANUAL"
      ble.sendCommandCheckOK("AT+HWModeLED=DISABLE");
    }
  }
  send_status_ble();
}
#endif


#ifdef SPO2_ENABLE
void handle_pulsoxy(){
  if (Serial1.available()) {
    pulsoxy_byte = Serial1.read();
    pulsoxy_syncbit = (pulsoxy_byte&1<<7)>>7;
    if (pulsoxy_syncbit == 1){
      pulseoxy_bcibyte = 0;
    }
    // parse bytes
    // (_, signalOK, probeOK, sensorOK, pulseOK, _, _, pulsoxyOK)
    if (pulseoxy_bcibyte == 0) {
      pulsoxy_signalstrength = pulsoxy_byte&7;  // 3 bits, 0-7
      // pulsoxy_signalOK = (pulsoxy_byte&1<<4)>>4; // 0=OK
      pulsoxy_status_bits = (pulsoxy_status_bits&0b10111111) | ((~pulsoxy_byte&0b00010000)<<2); // bit 4 into bit 6, invert
      // pulsoxy_probeOK = (pulsoxy_byte&1<<5)>>5; // 0=CONNECTED
      pulsoxy_status_bits = (pulsoxy_status_bits&0b11011111) | (~pulsoxy_byte&0b00100000); // bit 5 into bit 5, invert
      // pulsoxy_pulsebeep = (pulsoxy_byte&1<<6)>>6;
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 1){
      pulsoxy_pleth = (pulsoxy_byte&127); // first 7 bits
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 2){
      // pulsoxy_bargraph = pulsoxy_byte&7;
      // pulsoxy_sensorOK = (pulsoxy_byte&1<<4)>>4; // 0=OK
      pulsoxy_status_bits = (pulsoxy_status_bits&0b11101111) | (~pulsoxy_byte&0b00010000); // bit 4 into bit 4, invert
      // pulsoxy_pulseOK = (pulsoxy_byte&1<<5)>>5;  // 0=OK
      pulsoxy_status_bits = (pulsoxy_status_bits&0b11110111) | ((~pulsoxy_byte&0b00100000)>>2); // bit 5 into bit 3, invert
      pulsoxy_heartratebit7 = (pulsoxy_byte&1<<6)<<1;
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 3){
      pulsoxy_heartrate = pulsoxy_heartratebit7|(pulsoxy_byte&127); // bits 0-6 plus the 7th
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 4){
      pulsoxy_spo2 = (pulsoxy_byte&127); // first 7 bits, 0-100%
      if (pulsoxy_spo2 != 127 && pulsoxy_signalstrength > PULSOXY_MIN_SIGNAL) {
        pulsoxy_mode = true;
      } else {
        pulsoxy_mode = false;
      }
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 5){
      // not used
      pulseoxy_bcibyte += 1;
    } else if (pulseoxy_bcibyte == 6){
      // not used
      pulseoxy_bcibyte = -1;
      // end of pulsoxy frame
      // push all pulsoxy frame data
      frame_pulsoxy_signalstrength = pulsoxy_signalstrength;
      frame_pulsoxy_heartrate = pulsoxy_heartrate;
      frame_pulsoxy_spo2 = pulsoxy_spo2;
      frame_pulsoxy_status_bits = pulsoxy_status_bits;
      // set spo2LOW bit
      if (pulsoxy_spo2 < STATUS_SPO2_LOW) {
        // (_, _, bleFAIL, baroFAIL, breathFAST, breathLOW, batLOW, spo2LOW)
        status_bits = status_bits|0b00000001;
      } else {
        status_bits = status_bits&0b11111110;
      }
    } else {
      // out of sync
    }
  }
}
#endif


void send_status_serial() {
  // send via usbserial
  // for Arduino serial plotter
  if (status_step_serial == 0) {
    Serial.print(rhythm_get_strength());
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 1) {
    Serial.print(rhythm_get_period_ms());
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 2) {
    Serial.print(oxygen_pct);
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 3) {
    Serial.print((int)(oxygen_total_ms/1000));
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 4) {
    Serial.print(flight_level);
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 5) {
    Serial.print(round(bat_pct*100));
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 6) {
    Serial.print(frame_pulsoxy_signalstrength);
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 7) {
    Serial.print(frame_pulsoxy_heartrate);
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 8) {
    Serial.print(frame_pulsoxy_spo2);
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 9) {
    // explode status bits, map to 0 and 100 for visible plotting
    // (_, signalOK, probeOK, sensorOK, pulseOK, _, _, pulsoxyOK)
    //signalOK
    Serial.print(map((frame_pulsoxy_status_bits&0b01000000)>>6, 0, 1, 0, 100));
    Serial.print(" ");
    //probeOK
    Serial.print(map((frame_pulsoxy_status_bits&0b00100000)>>5, 0, 1, 0, 100));
    Serial.print(" ");
    //sensorOK
    Serial.print(map((frame_pulsoxy_status_bits&0b00010000)>>4, 0, 1, 0, 100));
    Serial.print(" ");
    //pulseOK
    Serial.print(map((frame_pulsoxy_status_bits&0b00001000)>>3, 0, 1, 0, 100));
    Serial.print(" ");
    //pulsoxyOK
    Serial.print(map((frame_pulsoxy_status_bits&0b00000001)>>0, 0, 1, 0, 100));
    Serial.print(" ");
    status_step_serial += 1;
  } else if (status_step_serial == 10) {
    // explode status bits, map to 0 and 100 for visible plotting
    // (_, _, bleFAIL, baroFAIL, breathFAST, breathLOW, batLOW, spo2LOW)
    // set breathLOW bit
    if (rhythm_get_strength() < STATUS_BREATH_LOW) {
      status_bits = status_bits|0b00000100;
    } else {
      status_bits = status_bits&0b11111011;
    }
    // set breathLOW bit
    if (rhythm_get_period_ms() < STATUS_BREATH_FAST) {
      status_bits = status_bits|0b00001000;
    } else {
      status_bits = status_bits&0b11110111;
    }
    //bleFAIL
    Serial.print(map((status_bits&0b00100000)>>5, 0, 1, 0, 100));
    Serial.print(" ");
    //baroFAIL
    Serial.print(map((status_bits&0b00010000)>>4, 0, 1, 0, 100));
    Serial.print(" ");
    //breathFAST
    Serial.print(map((status_bits&0b00001000)>>3, 0, 1, 0, 100));
    Serial.print(" ");
    //breathLOW
    Serial.print(map((status_bits&0b00000100)>>2, 0, 1, 0, 100));
    Serial.print(" ");
    //batLOW
    Serial.print(map((status_bits&0b00000010)>>1, 0, 1, 0, 100));
    Serial.print(" ");
    //spo2LOW
    Serial.print(map((status_bits&0b00000001), 0, 1, 0, 100));
    Serial.println(" ");  // println since last
    status_step_serial = 0;  // start over
  }
}

#ifdef BLE_ENABLE
void send_status_ble() {
  if (ble.isConnected()) {
    if (status_step_ble == 0) {
      ble.print(rhythm_get_strength());
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 1) {
      ble.print(rhythm_get_period_ms());
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 2) {
      ble.print(oxygen_pct);
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 3) {
      ble.print((int)(oxygen_total_ms/1000));
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 4) {
      ble.print(flight_level);
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 5) {
      ble.print(round(bat_pct*100));
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 6) {
      ble.print(frame_pulsoxy_signalstrength);
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 7) {
      ble.print(frame_pulsoxy_heartrate);
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 8) {
      ble.print(frame_pulsoxy_spo2);
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 9) {
      // (_, signalOK, probeOK, sensorOK, pulseOK, _, _, pulsoxyOK)
      ble.print(frame_pulsoxy_status_bits);
      ble.print(" ");
      status_step_ble += 1;
    } else if (status_step_ble == 10) {
      // (_, _, bleFAIL, baroFAIL, breathFAST, breathLOW, batLOW, spo2LOW)
      ble.println(status_bits);
      status_step_ble = 0;  // start over
    }
  }
}
#endif
