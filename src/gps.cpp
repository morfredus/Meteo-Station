// gps.cpp
#include "config.h"
#include "gps.h"
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

HardwareSerial GPS(1);
TinyGPSPlus gpsParser;

volatile bool ppsPulse = false;
unsigned long lastPpsMs = 0;

void IRAM_ATTR ppsISR() {
  ppsPulse = true;
  lastPpsMs = millis();
}

void gpsBegin() {
  GPS.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  pinMode(PIN_GPS_PPS, INPUT);
  attachInterrupt(PIN_GPS_PPS, ppsISR, RISING);
}

void gpsLoop(GpsFix &fix) {
  while (GPS.available() > 0) {
    char c = GPS.read();
    gpsParser.encode(c);
  }

  if (gpsParser.location.isUpdated() && gpsParser.location.isValid()) {
    fix.lat = gpsParser.location.lat();
    fix.lon = gpsParser.location.lng();
    fix.hasFix = true;
  } else {
    fix.hasFix = false;
  }

  fix.sats = gpsParser.satellites.value();
  fix.ppsLocked = (millis() - lastPpsMs) < 1500;
}