/*
  rhythm.c - breathing rhythm capture and analysis
  Part of the WaveGlide firmware. See: https://github.com/nortd/WaveGlide

  MIT License

  Copyright (c) 2020 Stefan Hechenberger

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


#include "Arduino.h"
#include "rhythm.h"


#define RHYTHM_VALS_SIZE 4
int vals[RHYTHM_VALS_SIZE] = {0};
uint8_t vals_cursor = 0;

#define RHYTHM_BASELINE_TOL_LOW 1
#define RHYTHM_BASELINE_TOL_HIGH 8
#define RHYTHM_BASELINE_DUR 1000/RHYTHM_TEMPRES  // dur for new baseline, 1s
#define RHYTHM_OFFLINE_DUR 6000/RHYTHM_TEMPRES   // time considered as canula off event
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

float breath_strength = 0;
uint8_t phase = PHASE_IDLE;

void rhythm_addval(int val) {
  // find feature and calculate period and phase

  // check for flat baseline
  if (val <= (baseline_last_ref + baseline_last_tol)
      && val >= (baseline_last_ref - baseline_last_tol)) {
    // val around baseline_last_ref
    baseline_flat_dur++;
    if (baseline_flat_dur > 48) {
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

  // calc strength
  breath_strength = 0.9*breath_strength + 0.1*abs(val-baseline);

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
    if (val_oldest+RHYTHM_FEATURE_SUCK > baseline
        && val_youngest+RHYTHM_FEATURE_SUCK < baseline
      ) {
    // if (baseline_flat_dur < 6
    //     && val_oldest+RHYTHM_FEATURE_SUCK > baseline
    //     && val_youngest+RHYTHM_FEATURE_SUCK < baseline
    //   ) {
      // found start of inhale
      phase = PHASE_INHALE;
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
    } else if (val_oldest > baseline && val_youngest > baseline) {
      // found start of exhale
      phase = PHASE_EXHALE;
    }
  }

  // housekeeping
  if (sample_count < 128) { // cap phase
    sample_count++;  // inc for every buffer entry
  }

  // canula/user offline detection
  if (baseline_flat_dur > RHYTHM_OFFLINE_DUR) {
    // found idle
    phase = PHASE_IDLE;
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



int rhythm_get_period_ms() {
  return last_period_smoothed*RHYTHM_TEMPRES;
}

uint8_t rhythm_get_phase() {
  return phase;
}

bool baseline_set() {
  return baseline_ref_set;
}

int get_baseline() {
  return baseline;
}

int rhythm_get_baseline() {
  return baseline;
}

uint8_t rhythm_get_strength() {
  // map [0,BREATH_STRENGTH_100] to [0,100]
  if (breath_strength > BREATH_STRENGTH_100) {return 100;}
  else if (breath_strength < 0) {return 0;}
  else {return round((100*breath_strength)/BREATH_STRENGTH_100);}
}
