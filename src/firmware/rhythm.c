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


#define RHYTHM_BUFFER_SIZE 128  // must be less than uint8_t
int buffer[RHYTHM_BUFFER_SIZE] = {0};
uint8_t buffer_cursor = 0;

#define RHYTHM_VALS_SIZE 6
#define RHYTHM_VALS_SMALL_DELTA 4
#define RHYTHM_VALS_BIG_DELTA 3
int vals[RHYTHM_VALS_SIZE] = {0};
uint8_t vals_cursor = 0;

#define RHYTHM_ANALYZE_EVERY 10  // must be < RHYTHM_BUFFER_SIZE
#define RHYTHM_OFFSET_PCT 0.0    // trigger feature to inhalation start pct
#define RHYTHM_INHALE_PCT 0.3    // period percentage useful for oxygenation
#define RHYTHM_FEATURE_SIZE 32
uint8_t feature_distances[RHYTHM_FEATURE_SIZE] = {0};
uint8_t feature_cursor = 0;
uint8_t addval_count = 0;
uint8_t analyze_cursor = 0;
uint8_t feature_analyze_cursor = 0;
uint8_t sample_count = 0;
float period_smoothed = 4000.0/RHYTHM_TEMPRES;  // start with 40 samples (4s)
uint8_t phase_smpl = 0;
float phase_pct = 0.0;  // current phase position in percent


void compute_period_and_phase();


// void rhythm_init() {
//   for (int i=0; i<RHYTHM_BUFFER_SIZE; i++) {
//     buffer[i] = 0;
//   }
// }

uint8_t rhythm_addval(int val) {
  addval_count++;

  // add to buffer
  uint8_t ret_cursor = buffer_cursor;
  buffer[buffer_cursor] = val;
  if (++buffer_cursor == RHYTHM_BUFFER_SIZE) { buffer_cursor = 0; } // inc, wrap

  // analyze periodicity
  if (addval_count > RHYTHM_ANALYZE_EVERY) {  // call every so often
    compute_period_and_phase();
  }

  return ret_cursor;
}


bool rhythm_oxygen(float dur_pct) {
  compute_period_and_phase();
  if (phase_pct > RHYTHM_OFFSET_PCT
    && phase_pct < (RHYTHM_OFFSET_PCT + dur_pct*RHYTHM_INHALE_PCT)) {
    return true;
  } else {
    return false;
  }
}


// bool rhythm_inhale_start() {
//   compute_period_and_phase();
//   if (phase_pct > RHYTHM_TRIGGER_PCT && !triggered) {
//     triggered = true;
//     return true;
//   } else {
//     return false;
//   }
// }


int rhythm_getgraph_y(int val) {
  return (val-525) * 0.5;
}

uint8_t rhythm_get_period() {
  return (uint8_t)round(period_smoothed);
}

uint8_t rhythm_get_phase() {
  return phase_smpl;
}


void compute_period_and_phase() {
  // side effects:
  //    vals[]
  //    vals_cursor
  //    sample_count
  //    period_smoothed
  //    phase_smpl
  //    phase_pct
  addval_count = 0;

  // features - find repeating falling ridge
  // loop through buffer from analyze_cursor to (excluding) buffer_cursor
  while (analyze_cursor != buffer_cursor) {
    // add val to vals
    vals[vals_cursor] = buffer[analyze_cursor];
    if (++vals_cursor == RHYTHM_VALS_SIZE) { vals_cursor = 0; } // inc, wrap
    // analyze vals, starting with oldest
    int first_val = 0;
    int last_val = 0;
    uint8_t small_delta_ok_num = 0;
    uint8_t k = vals_cursor;
    do {
      if (k == vals_cursor) { // first run
        first_val = vals[k];
      } else {               // not first run
        // small delta check
        if (abs(last_val - vals[k]) < RHYTHM_VALS_SMALL_DELTA) {
          small_delta_ok_num++;
        }
      }
      last_val = vals[k];
      if (++k == RHYTHM_VALS_SIZE) { k = 0; } // inc, wrap
    } while (k != vals_cursor);
    // analyze vals, check deltas
    // if ((first_val - last_val) > RHYTHM_VALS_BIG_DELTA
    //     && small_delta_ok_num == RHYTHM_VALS_SIZE - 1
    //     && sample_count > 0.5*period_smoothed) {
    if (first_val > 525 && last_val < 525 && sample_count > 0.5*period_smoothed) {
      // found falling ridge
      feature_distances[feature_cursor] = sample_count;
      sample_count = 0;
      if (++feature_cursor == RHYTHM_FEATURE_SIZE) { feature_cursor = 0; } // inc, wrap
    }
    // housekeeping
    sample_count++;  // inc for every buffer entry
    if (++analyze_cursor == RHYTHM_BUFFER_SIZE) { analyze_cursor = 0; } // inc, wrap
  } // next analysis starts from buffer_cursor

  // period - accumulated average
  // from feature_analyze_cursor to feature_cursor (ex)
  while (feature_analyze_cursor != feature_cursor) {
    int period = feature_distances[feature_analyze_cursor];
    period_smoothed = 0.2*period_smoothed + 0.8*period;
    if (++feature_analyze_cursor == RHYTHM_FEATURE_SIZE) { feature_analyze_cursor = 0; } // inc, wrap
  } // next analysis starts from feature_cursor

  // phase - progression percentage
  if (period_smoothed != 0) {  // sanity check
    phase_smpl = sample_count;
    phase_pct = sample_count/period_smoothed;
  }

}
