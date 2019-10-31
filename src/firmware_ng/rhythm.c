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

#define RHYTHM_BASELINE_TOL_LOW 2
#define RHYTHM_BASELINE_TOL_HIGH 8
#define RHYTHM_BASELINE_DUR 1000/RHYTHM_TEMPRES  // dur for new baseline, 1s
#define RHYTHM_OFFLINE_DUR 4000/RHYTHM_TEMPRES   // time considered as canula off event
int baseline = 512;  // baseline sensor read (when not breathing)
int baseline_last_ref = 512;
bool baseline_ref_set = false;
uint16_t baseline_flat_dur = 0;
uint8_t baseline_last_tol = RHYTHM_BASELINE_TOL_LOW;

#define RHYTHM_OFFSET_PCT 0.03   // trigger feature to inhalation start pct
#define RHYTHM_INHALE_PCT 0.32    // period percentage useful for oxygenation
#define RHYTHM_FEATURE_SUCK 3
// #define RHYTHM_COUNT_MAX_DEFAULT 20  // 2s
#define RHYTHM_LAST_PERIOD_DEFAULT 4000.0/RHYTHM_TEMPRES;  // init at 40 samples (4s)
uint8_t sample_count = 0;
// uint8_t rhythm_count_max = RHYTHM_COUNT_MAX_DEFAULT;
uint8_t feature_dedection_delayer = 0;
uint8_t last_period = RHYTHM_LAST_PERIOD_DEFAULT;
float last_period_smoothed = RHYTHM_LAST_PERIOD_DEFAULT;
float phase_pct = 0.0;     // current phase position in percent
float duration_pct = 0.0;  // oxygenation percentage as requested

bool on_cycle_flag = false;
uint8_t oxygen_cycle_count = 0;
uint8_t oxygen_every = 1;


void rhythm_addval(int val) {
  // find feature and calculate period and phase

  // check for flat baseline
  if (val <= (baseline_last_ref + baseline_last_tol)
      && val >= (baseline_last_ref - baseline_last_tol)) {
    // val around baseline_last_ref
    baseline_flat_dur++;
    if (baseline_flat_dur > 24) {
      // reduce sensitivity
      baseline_last_tol = RHYTHM_BASELINE_TOL_HIGH;
    }
    if (!baseline_ref_set && (baseline_flat_dur == RHYTHM_BASELINE_DUR)) {
      // got new baseline
      baseline = baseline_last_ref;
      baseline_ref_set = true;
    }
  } else {
    baseline_flat_dur = 0;
    baseline_last_ref = val;
    baseline_last_tol = RHYTHM_BASELINE_TOL_LOW;
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
      last_period_smoothed = 0.7*last_period_smoothed + 0.3*last_period;
      sample_count = 0;
      feature_dedection_delayer = 0;
    }
  }

  // housekeeping
  if (sample_count < 128) { // cap phase
    sample_count++;  // inc for every buffer entry
  }

  // canula/user offline detection
  if (baseline_flat_dur > RHYTHM_OFFLINE_DUR) {
    // rhythm_count_max = RHYTHM_COUNT_MAX_DEFAULT;
  }

  // phase
  phase_pct = (float)sample_count/last_period_smoothed;  // phase progression in percent

}


bool rhythm_oxygen(int dur_pct) {
  bool ret = false;
  float offset_factor = 1.0;
  // set mode
  if (dur_pct < 15) {  // when duration is < 15% use quadro mode
    // mode 4: 4x the oxygen on every 4th inhalation
    duration_pct = dur_pct/25.0;  // convert to 0-1.0, also x4
    oxygen_every = 4;
    offset_factor = 2.0;
  } else if (dur_pct < 40) {  // when duration is < 40% use double mode
    // mode 2: twice the oxygen on every other inhalation
    duration_pct = dur_pct/50.0;  // convert to 0-1.0, also double
    oxygen_every = 2;
    offset_factor = 1.5;
  } else {
    // mode 1: oxygen on every inhaltion
    duration_pct = dur_pct/100.0;
    oxygen_every = 1;
  }
  // evaluate oxygen yes/no
  if (baseline_ref_set
    && phase_pct >= RHYTHM_OFFSET_PCT*offset_factor
    && phase_pct < (RHYTHM_OFFSET_PCT*offset_factor + duration_pct*RHYTHM_INHALE_PCT)) {
    // ON state
    on_cycle_flag = true;
    // oxygenation phase
    if ((oxygen_cycle_count % oxygen_every) != 0) {
      // skip this cycle
      ret = false;
    } else {
      ret = true;
    }
  } else {
    // OFF state
    if (on_cycle_flag) {
      oxygen_cycle_count++;
      on_cycle_flag = false;
    }
    // inc feature_dedection_delayer, also keep at bay
    if (++feature_dedection_delayer == 128) { feature_dedection_delayer = 0; }
    ret = false;
  }
  return ret;
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
