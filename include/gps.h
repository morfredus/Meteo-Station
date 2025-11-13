// gps.h
#pragma once
#include <Arduino.h>

struct GpsFix {
  bool hasFix;
  double lat;
  double lon;
  uint8_t sats;
  bool ppsLocked;
};

void gpsBegin();
void gpsLoop(GpsFix &fix);