// telemetry.cpp
#include "config.h"
#include "telemetry.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "weather.h"

extern WeatherData gWeather;
extern float gTempInt;
extern float gHumInt;
extern double gLat;
extern double gLon;
extern bool gUseDefaultGeo;

void telegramSend(const String &msg) {
  if (WiFi.status()!=WL_CONNECTED) return;
  HTTPClient http;
  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String payload = String("{\"chat_id\":\"") + TELEGRAM_CHAT_ID + "\",\"text\":\"" + msg + "\"}";
  http.POST(payload);
  http.end();
}

String formatWeatherBrief() {
  String s = "Meteo: " + String(gWeather.now.tempNow,1) + "°C, ";
  s += "hum " + String(gWeather.now.humidity,0) + "%, vent " + String(gWeather.now.wind,1) + " m/s\n";
  s += "Prévision: min " + String(gWeather.now.tempMin,1) + "°C / max " + String(gWeather.now.tempMax,1) + "°C\n";
  if (gWeather.now.hasAlert) {
    s += "Alerte: " + gWeather.now.alertTitle + " (" + gWeather.now.alertSeverity + ")\n";
  } else {
    s += "Pas d’alerte.\n";
  }
  s += "Intérieur: " + String(gTempInt,1) + "°C, " + String(gHumInt,0) + "%\n";
  s += "Geo: " + String(gLat,5) + ", " + String(gLon,5) + (gUseDefaultGeo ? " (défaut Bordeaux)" : " (GPS)");
  return s;
}

static unsigned long lastPoll = 0;

void telegramLoop() {
  if (WiFi.status()!=WL_CONNECTED) return;
  if (millis() - lastPoll < 2500) return;
  lastPoll = millis();

  // Get updates (polling simple)
  HTTPClient http;
  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/getUpdates";
  http.begin(url);
  int code = http.GET();
  if (code!=200) { http.end(); return; }
  String resp = http.getString();
  http.end();

  // Minimal parse for text commands
  // For robust behavior, track update_id and offset; here we simplify.
  if (resp.indexOf("/meteo")>=0) telegramSend(formatWeatherBrief());
  if (resp.indexOf("/temp")>=0) telegramSend("Temp interieur: " + String(gTempInt,1) + "°C");
  if (resp.indexOf("/hygro")>=0) telegramSend("Hygrometrie: " + String(gHumInt,0) + "%");
  if (resp.indexOf("/alertes")>=0) {
    if (gWeather.now.hasAlert) telegramSend("Alerte: " + gWeather.now.alertTitle + "\n" + gWeather.now.alertDesc);
    else telegramSend("Pas d’alerte en cours.");
  }
  if (resp.indexOf("/geo")>=0) {
    telegramSend("Position: " + String(gLat,5) + ", " + String(gLon,5) + (gUseDefaultGeo ? " (défaut Bordeaux)" : " (GPS)"));
  }
  if (resp.indexOf("/reboot")>=0) {
    telegramSend("Redémarrage demandé.");
    delay(500);
    esp_restart();
  }
}
