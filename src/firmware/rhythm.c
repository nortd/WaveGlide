/*
  rhythm.c - breathing rhythm capture and analysis
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


#include "Arduino.h"
#include "rhythm.h"


#define RHYTHM_VALS_SIZE 4
// #define RHYTHM_VALS_SMALL_DELTA 1
#define RHYTHM_VALS_BIG_DELTA 50
int vals[RHYTHM_VALS_SIZE] = {0};
uint8_t vals_cursor = 0;

#define RHYTHM_BASELINE_TOL 2
#define RHYTHM_BASELINE_DUR 1000/RHYTHM_TEMPRES  // dur for new baseline, 1s
int baseline = 512;  // baseline sensor read (when not breathing)
int baseline_ref = 512;
bool baseline_ref_set = false;
uint16_t baseline_flat_dur = 0;

#define RHYTHM_OFFSET_PCT 0.0    // trigger feature to inhalation start pct
#define RHYTHM_INHALE_PCT 0.3    // period percentage useful for oxygenation
#define RHYTHM_FEATURE_SIZE 32
#define RHYTHM_FEATURE_SUCK 3
// #define RHYTHM_PERIOD_MIN 1000.0/RHYTHM_TEMPRES  // 1s, must not be 0 because /0
// #define RHYTHM_PERIOD_MAX 8000.0/RHYTHM_TEMPRES  // 8s
#define RHYTHM_COUNT_MAX 128
#define RHYTHM_OXYGEN_OFF_DELAY 10000
uint8_t sample_count = 0;
bool feature_dedection_deactivated = false;
uint8_t feature_dedection_countdown = 0;
unsigned long oxygen_off_time = 0;
unsigned long oxygen_off_delay = 0;
bool oxygen_last = false;
float period_smoothed = 4000.0/RHYTHM_TEMPRES;  // start with 40 samples (4s)
uint8_t phase_smpl = 0;
float phase_pct = 0.0;  // current phase position in percent
float duration_pct = 0.0;



void rhythm_addval(int val) {
  // find feature and calculate period and phase

  // check for flat baseline
  if (val <= (baseline_ref + RHYTHM_BASELINE_TOL)
      && val >= (baseline_ref - RHYTHM_BASELINE_TOL)) {
    baseline_flat_dur++;
    if (!baseline_ref_set && baseline_flat_dur == RHYTHM_BASELINE_DUR) {
      // got new baseline
      baseline = baseline_ref;
      baseline_ref_set = true;
    }
  } else {
    baseline_flat_dur = 0;
    baseline_ref = val;
  }

  // handle delay after oxygen burst
  // oxygen_off_delay = millis()-oxygen_off_time;
  // if (oxygen_off_delay > RHYTHM_OXYGEN_OFF_DELAY) {
  //   feature_dedection_deactivated = false;
  // }

  // add val to vals
  vals[vals_cursor] = val;
  if (++vals_cursor == RHYTHM_VALS_SIZE) { vals_cursor = 0; } // inc, wrap

  // analyze vals, starting with oldest
  int first_val = 0;
  int last_val = 0;
  // uint8_t small_delta_ok_num = 0;
  uint8_t k = vals_cursor;
  do {
    if (k == vals_cursor) { // first run
      first_val = vals[k];
    } else {               // not first run
      // // small delta check
      // if (abs(last_val - vals[k]) < RHYTHM_VALS_SMALL_DELTA) {
      //   small_delta_ok_num++;
      // }
    }
    last_val = vals[k];
    if (++k == RHYTHM_VALS_SIZE) { k = 0; } // inc, wrap
  } while (k != vals_cursor);

  // check for period feature
  // if ((first_val - last_val) > RHYTHM_VALS_BIG_DELTA
  //     && small_delta_ok_num == RHYTHM_VALS_SIZE - 1
  //     && sample_count > 0.5*period_smoothed)
  if (feature_dedection_countdown > 4
      && first_val+RHYTHM_FEATURE_SUCK > baseline
      && last_val+RHYTHM_FEATURE_SUCK < baseline
      && baseline_flat_dur < 6
      // && sample_count > sample_count_oxygen_off+3
      // && (sample_count/period_smoothed) > (RHYTHM_OFFSET_PCT + duration_pct*RHYTHM_INHALE_PCT)
      // && (first_val - last_val) < RHYTHM_VALS_BIG_DELTA
    ) {
    // found end of exhale
    // period_smoothed = 0.8*period_smoothed + 0.2*sample_count;  // smooth a bit
    period_smoothed = sample_count;
    sample_count = 0;
  }

  // housekeeping
  if (sample_count < RHYTHM_COUNT_MAX) { // cap phase
    sample_count++;  // inc for every buffer entry
  }

  // phase
  phase_smpl = sample_count;  // samples since last period feature
  phase_pct = sample_count/period_smoothed;  // phase progression in percent

}


bool rhythm_oxygen(int dur_pct) {
  duration_pct = dur_pct/100.0;
  if (phase_pct >= RHYTHM_OFFSET_PCT
    && phase_pct < (RHYTHM_OFFSET_PCT + duration_pct*RHYTHM_INHALE_PCT)) {
    // oxygenation phase
    // feature_dedection_deactivated = true;
    feature_dedection_countdown = 0;
    // oxygen_last = true;
    return true;
  } else {
    feature_dedection_countdown++;
    // // non-oxygenation phase
    // if (oxygen_last) {
    //   // oxygen_off_time = millis();
    //
    // }
    // oxygen_last = false;
    return false;
  }
}



uint8_t rhythm_get_period() {
  return (uint8_t)round(period_smoothed);
}

uint8_t rhythm_get_phase() {
  return phase_smpl;
}

bool baseline_set() {
  return baseline_ref_set;
}

int rhythm_get_baseline() {
  return baseline;
}

bool no_feature_dedection() {
  return feature_dedection_deactivated;
}
