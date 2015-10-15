/*
  rhythm.h - breathing rhythm capture and analysis
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


#ifndef rhythm_h
#define rhythm_h

#define RHYTHM_TEMPRES 100  // temporal resolution in ms

// void rhythm_init();

uint8_t rhythm_addval(int val);
bool rhythm_oxygen(float dur_pct);
int rhythm_get_baseline();
uint8_t rhythm_get_period();
uint8_t rhythm_get_phase();



#endif
