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


#define RHYTHM_BUFFER_SIZE 128  // must be less than uint8_t
int buffer[RHYTHM_BUFFER_SIZE] = {0};
uint8_t buffer_cursor = 0;

#define RHYTHM_VALS_SIZE 4
#define RHYTHM_VALS_SMALL_DELTA 5
#define RHYTHM_VALS_BIG_DELTA 10
int vals[RHYTHM_VALS_SIZE] = {0};
uint8_t vals_cursor = 0;

#define RHYTHM_FEATURE_SIZE 32
uint8_t feature_found_at_dist[RHYTHM_FEATURE_SIZE];

#define RHYTHM_TRIGGER_PCT 0.15

// void rhythm_init() {
//   for (int i=0; i<RHYTHM_BUFFER_SIZE; i++) {
//     buffer[i] = 0;
//   }
// }

uint8_t rhythm_addval(int val) {
  uint8_t temp_cursor = buffer_cursor;
  buffer[buffer_cursor] = val;
  if (++buffer_cursor == RHYTHM_BUFFER_SIZE) { buffer_cursor = 0; } // inc, wrap
  return temp_cursor;
}



bool rhythm_inhale_start() {
  bool ret = false;
  feature_found_at_dist = {0};
  // loop through buffer, starting with oldest
  uint8_t feature_idx = 0;
  uint8_t feature_dist = 0;
  uint8_t i = buffer_cursor;
  do {
    // add val to vals
    vals[vals_cursor] = buffer[i];
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
    if ((first_val - last_val) > RHYTHM_VALS_BIG_DELTA
        && small_delta_ok_num ==  RHYTHM_VALS_SIZE - 1) {
      // found falling ridge
      if (feature_idx < RHYTHM_FEATURE_SIZE) {
        feature_found_at_dist[feature_idx++] = feature_dist;
      }
    }
    // housekeeping
    feature_dist++;
    if (++i == RHYTHM_BUFFER_SIZE) { i = 0; } // inc, wrap
  } while (i != buffer_cursor);
  // analyze feature distances
  int period = 0.0;
  float period_smoothed = 0.0;  // smoothed following
  if (feature_idx-1 > 0) {  // at least two entries
    for (uint8_t d = 1; d < feature_idx-1; d++) {
      period = feature_found_at_dist[d] - feature_found_at_dist[d-1];
      if (d == 1) {  // firstly, assign
        period_smoothed = period;
      } else {       // consecutively, accumulated average
        period_smoothed = 0.4*period_smoothed + 0.6*period;
      }
    }
  }
  // analyze phase
  if (feature_idx-1 > 0) {  // at least two entries, implies period calc'ed
    uint8_t dist_from_last = feature_dist-feature_found_at_dist[feature_idx-1];
    if (dist_from_last > RHYTHM_TRIGGER_PCT*period) {
      rhythm_trigger_flag = true;
      ret = true;
    }
  }
  // finally an answer
  return ret;
}



int rhythm_period_dur() {

}

int rhythm_getgraph_y(int val) {
  return (val-540) * 0.5;
}
