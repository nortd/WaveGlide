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
int baseline_ref = 512;
bool baseline_ref_set = false;
uint16_t baseline_flat_dur = 0;
uint16_t baseline_flat_dur_prev = 0;

#define RHYTHM_OFFSET_PCT 0.0    // trigger feature to inhalation start pct
#define RHYTHM_INHALE_PCT 0.4    // period percentage useful for oxygenation
#define RHYTHM_FEATURE_SUCK 3
#define RHYTHM_COUNT_DEFAULT 20  // 2s
uint8_t sample_count = 0;
uint8_t rhythm_count_max = RHYTHM_COUNT_DEFAULT;
uint8_t feature_dedection_delayer = 0;
float period_smoothed = 4000.0/RHYTHM_TEMPRES;  // start with 40 samples (4s)
uint8_t phase_smpl = 0;
float phase_pct = 0.0;  // current phase position in percent
float duration_pct = 0.0;



void rhythm_addval(int val) {
  // find feature and calculate period and phase

  // check for flat baseline
  if (val <= (baseline_ref + RHYTHM_BASELINE_TOL)
      && val >= (baseline_ref - RHYTHM_BASELINE_TOL)) {
    baseline_flat_dur_prev = baseline_flat_dur;
    baseline_flat_dur++;
    if (!baseline_ref_set && baseline_flat_dur == RHYTHM_BASELINE_DUR) {
      // got new baseline
      baseline = baseline_ref;
      baseline_ref_set = true;
    }
  } else {
    baseline_flat_dur_prev = baseline_flat_dur;
    baseline_flat_dur = 0;
    baseline_ref = val;
  }

  // add val to vals
  vals[vals_cursor] = val;
  if (++vals_cursor == RHYTHM_VALS_SIZE) { vals_cursor = 0; } // inc, wrap

  // analyze vals, starting with oldest
  int first_val = 0;
  int last_val = 0;
  uint8_t k = vals_cursor;
  do {
    if (k == vals_cursor) { // first run
      first_val = vals[k];
    } else {               // not first run
    }
    last_val = vals[k];
    if (++k == RHYTHM_VALS_SIZE) { k = 0; } // inc, wrap
  } while (k != vals_cursor);

  // check for period feature
  if (baseline_ref_set
      && feature_dedection_delayer > RHYTHM_VALS_SIZE
      && first_val+RHYTHM_FEATURE_SUCK > baseline
      && last_val+RHYTHM_FEATURE_SUCK < baseline
      && baseline_flat_dur_prev < 6
    ) {
    // found end of exhale
    // period_smoothed = 0.8*period_smoothed + 0.2*sample_count;  // smooth a bit
    period_smoothed = sample_count;
    rhythm_count_max = 2*sample_count;  // only increase after first feature
    sample_count = 0;
    feature_dedection_delayer = 0;
  }

  // housekeeping
  if (sample_count < rhythm_count_max) { // cap phase
    sample_count++;  // inc for every buffer entry
  }

  if (baseline_flat_dur > RHYTHM_OFFLINE_DUR) {
    rhythm_count_max = RHYTHM_COUNT_DEFAULT;
  }

  // phase
  phase_smpl = sample_count;  // samples since last period feature
  phase_pct = sample_count/period_smoothed;  // phase progression in percent

}


bool rhythm_oxygen(int dur_pct) {
  duration_pct = dur_pct/100.0;
  if (baseline_ref_set
    && phase_pct >= RHYTHM_OFFSET_PCT
    && phase_pct < (RHYTHM_OFFSET_PCT + duration_pct*RHYTHM_INHALE_PCT)) {
    // oxygenation phase
    return true;
  } else {
    feature_dedection_delayer++;
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
