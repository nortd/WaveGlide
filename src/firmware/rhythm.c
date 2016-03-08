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
int vals[RHYTHM_VALS_SIZE] = {0};
uint8_t vals_cursor = 0;

#define RHYTHM_BASELINE_TOL 3
#define RHYTHM_BASELINE_DUR 1000/RHYTHM_TEMPRES  // dur for new baseline, 1s
#define RHYTHM_OFFLINE_DUR 4000/RHYTHM_TEMPRES   // time considered as canula off event
int baseline = 512;  // baseline sensor read (when not breathing)
int baseline_last_ref = 512;
bool baseline_ref_set = false;
uint16_t baseline_flat_dur = 0;
uint8_t baseline_last_tol = RHYTHM_BASELINE_TOL;

#define RHYTHM_OFFSET_PCT 0.03   // trigger feature to inhalation start pct
#define RHYTHM_INHALE_PCT 0.32    // period percentage useful for oxygenation
#define RHYTHM_FEATURE_SUCK 3
// #define RHYTHM_COUNT_MAX_DEFAULT 20  // 2s
#define RHYTHM_LAST_PERIOD_DEFAULT 4000.0/RHYTHM_TEMPRES;  // init at 40 samples (4s)
uint8_t sample_count = 0;
// uint8_t rhythm_count_max = RHYTHM_COUNT_MAX_DEFAULT;
uint8_t feature_dedection_delayer = 0;
uint8_t last_period = RHYTHM_LAST_PERIOD_DEFAULT;
float phase_pct = 0.0;     // current phase position in percent
float duration_pct = 0.0;  // oxygenation percentage as requested



void rhythm_addval(int val) {
  // find feature and calculate period and phase

  // check for flat baseline
  if (val <= (baseline_last_ref + baseline_last_tol)
      && val >= (baseline_last_ref - baseline_last_tol)) {
    // val around baseline_last_ref
    baseline_flat_dur++;
    if (!baseline_ref_set && (baseline_flat_dur == RHYTHM_BASELINE_DUR)) {
      // got new baseline
      baseline = baseline_last_ref;
      baseline_ref_set = true;
      // increase baseline tolerance
      // prevents mis-oxygenation from sensor noise
      baseline_last_tol += 4;
    }
  } else {
    baseline_flat_dur = 0;
    baseline_last_ref = val;
  }

  // add val to vals
  vals[vals_cursor] = val;
  if (++vals_cursor == RHYTHM_VALS_SIZE) { vals_cursor = 0; } // inc, wrap

  // analyze val trail for feature
  if (baseline_ref_set && feature_dedection_delayer > RHYTHM_VALS_SIZE) {
    // get oldest and youngest val (of val trail)
    int val_oldest = 0;
    int val_youngest = 0;
    uint8_t k = vals_cursor;
    do {
      if (k == vals_cursor) {
        val_oldest = vals[k];
      }
      val_youngest = vals[k];
      if (++k == RHYTHM_VALS_SIZE) { k = 0; } // inc, wrap
    } while (k != vals_cursor);

    // check for period feature
    if (baseline_flat_dur < 6
        && val_oldest+RHYTHM_FEATURE_SUCK > baseline
        && val_youngest+RHYTHM_FEATURE_SUCK < baseline
      ) {
      // found end of exhale
      // set last_period, cap
      if (sample_count > 1.5*last_period) {
        last_period = floor(1.5*last_period);
      } else if (sample_count < 0.5*last_period) {
        last_period = ceil(0.5*last_period);
      } else {
        last_period = sample_count;
      }
      sample_count = 0;
      // rhythm_count_max = min(2*last_period, 128);  // only increase after first feature
      feature_dedection_delayer = 0;
    }
  }

  // housekeeping
  if (sample_count < 128) { // cap phase
    sample_count++;  // inc for every buffer entry
  }
  // else {
  //   // assume offline
  //   sample_count = 0;
  //   last_period = RHYTHM_LAST_PERIOD_DEFAULT;
  //   // rhythm_count_max = RHYTHM_COUNT_MAX_DEFAULT;
  // }

  // canula/user offline detection
  if (baseline_flat_dur > RHYTHM_OFFLINE_DUR) {
    // rhythm_count_max = RHYTHM_COUNT_MAX_DEFAULT;
  }

  // phase
  phase_pct = (float)sample_count/(float)last_period;  // phase progression in percent

}


bool rhythm_oxygen(int dur_pct) {
  duration_pct = dur_pct/100.0;
  if (baseline_ref_set
    && phase_pct >= RHYTHM_OFFSET_PCT
    && phase_pct < (RHYTHM_OFFSET_PCT + duration_pct*RHYTHM_INHALE_PCT)) {
    // oxygenation phase
    return true;
  } else {
    // inc feature_dedection_delayer, also keep at bay
    if (++feature_dedection_delayer == 128) { feature_dedection_delayer = 0; }
    return false;
  }
}



uint8_t rhythm_get_period() {
  return last_period;
}

uint8_t rhythm_get_phase() {
  return sample_count;
}

bool baseline_set() {
  return baseline_ref_set;
}

int rhythm_get_baseline() {
  return baseline;
}
