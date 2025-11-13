// weather.cpp
#include "config.h"
#include "weather.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

String mapOWCodeToSimple(int id) {
  if (id>=200 && id<300) return "storm";
  if (id>=300 && id<600) return "rain";
  if (id>=600 && id<700) return "snow";
  if (id==701 || id==741) return "fog";
  if (id==800) return "clear";
  if (id>800 && id<900) return "clouds";
  return "clouds";
}

String severityFromAlert(const String &txt) {
  String l = txt; l.toLowerCase();
  if (l.indexOf("red")>=0 || l.indexOf("rouge")>=0) return "red";
  if (l.indexOf("orange")>=0) return "orange";
  if (l.indexOf("yellow")>=0 || l.indexOf("jaune")>=0) return "yellow";
  return "yellow";
}

bool fetchWeatherOpenWeather(float lat, float lon, WeatherData &out) {
  if (WiFi.status()!=WL_CONNECTED) return false;
  HTTPClient http;
  String url = String("https://api.openweathermap.org/data/2.5/onecall?lat=") + String(lat,6) +
               "&lon=" + String(lon,6) +
               "&units=metric&lang=fr&exclude=minutely&appid=" + TOKEN_OPENWEATHER;
  http.begin(url);
  int code = http.GET();
  if (code!=200) { http.end(); return false; }
  DynamicJsonDocument doc(32*1024);
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) return false;

  // current
  JsonObject cur = doc["current"];
  out.now.tempNow = cur["temp"] | NAN;
  out.now.humidity = cur["humidity"] | NAN;
  out.now.wind = cur["wind_speed"] | NAN;

  int wid = 0;
  if (cur["weather"][0]["id"].is<int>()) wid = cur["weather"][0]["id"].as<int>();
  out.now.conditionCode = mapOWCodeToSimple(wid);

  // daily today
  JsonObject d0 = doc["daily"][0];
  out.now.tempMin = d0["temp"]["min"] | NAN;
  out.now.tempMax = d0["temp"]["max"] | NAN;

  // forecast next 3 days
  for (int i=0;i<4;i++) {
    JsonObject d = doc["daily"][i];
    out.daily[i].date = String((long)(d["dt"] | 0));
    out.daily[i].tmin = d["temp"]["min"] | NAN;
    out.daily[i].tmax = d["temp"]["max"] | NAN;
    int widDay = d["weather"][0]["id"] | 800;
    out.daily[i].code = mapOWCodeToSimple(widDay);
    out.daily[i].pop = (d["pop"] | 0.0f) * 100.0f;
    out.daily[i].wind = d["wind_speed"] | 0.0f;
  }

  // alerts
  if (doc.containsKey("alerts")) {
    JsonObject a0 = doc["alerts"][0];
    out.now.hasAlert = true;
    out.now.alertTitle = (const char*)a0["event"] | "Alerte météo";
    out.now.alertDesc = (const char*)a0["description"] | "Voir détails";
    out.now.alertSeverity = severityFromAlert(out.now.alertTitle + " " + out.now.alertDesc);
  } else {
    out.now.hasAlert = false;
    out.now.alertTitle = "";
    out.now.alertDesc = "";
    out.now.alertSeverity = "";
  }

  return true;
}
