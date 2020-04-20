/*
  rhythm.h - breathing rhythm capture and analysis
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


#ifndef rhythm_h
#define rhythm_h

#define RHYTHM_TEMPRES 100  // temporal resolution in ms
#define BREATH_STRENGTH_100 20  // what is considered 100%
#define PHASE_INHALE 1
#define PHASE_EXHALE 2
#define PHASE_IDLE 3

// void rhythm_init();

void rhythm_addval(int val);
bool rhythm_oxygen(int dur_pct);
int rhythm_get_period_ms();
uint8_t rhythm_get_phase();
bool baseline_set();
int get_baseline();
int rhythm_get_baseline();
uint8_t rhythm_get_strength();  // average recent breath amplitude mapped to 0-100

#endif
