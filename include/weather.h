// weather.h
#pragma once
#include <Arduino.h>

struct WeatherNow {
  String conditionCode;   // "clear","clouds","rain","storm","fog","snow"
  float tempNow;          // prévision instantanée (OpenWeather current)
  float tempMin;
  float tempMax;
  float humidity;
  float wind;
  bool hasAlert;
  String alertTitle;
  String alertDesc;
  String alertSeverity;   // "yellow","orange","red"
};

struct ForecastDay {
  String date;
  float tmin;
  float tmax;
  String code;
  float pop; // prob of precip
  float wind;
};

struct WeatherData {
  WeatherNow now;
  ForecastDay daily[4]; // today + 3 jours
};

bool fetchWeatherOpenWeather(float lat, float lon, WeatherData &out);
String mapOWCodeToSimple(int owId);
String severityFromAlert(const String &providerText);
