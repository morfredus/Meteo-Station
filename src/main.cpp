// ===============================================
// Station Météo ESP32-S3
// Version: 1.0.03-dev
// v1.0.01-dev - Correction compilation ESP32-S3 : AsyncTCP et ArduinoJson 7
// v1.0.02-dev - Désactivation ESPAsyncWebServer (non utilisé, conflit WiFiServer.h)
// v1.0.03-dev - Framework ESP32 stable 6.8.1 (corrige bugs WiFiClientSecure/HTTPClient)
// ===============================================

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include "DHT.h"

#include "config.h"
#include "ui_icons.h"
#include "weather.h"
#include "gps.h"
#include "telemetry.h"

WiFiMulti wifiMulti;

// TFT et capteurs
Adafruit_ST7789 tft = Adafruit_ST7789(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
DHT dht(PIN_DHT, DHT22); // bascule vers DHT11 si besoin

WeatherData gWeather;
float gTempInt = NAN, gHumInt = NAN;
double gLat = DEFAULT_LAT, gLon = DEFAULT_LON;
bool gUseDefaultGeo = true;

enum Page { PAGE_HOME, PAGE_FORECAST, PAGE_ALERT, PAGE_SENSORS, PAGE_SYSTEM };
Page currentPage = PAGE_HOME;

unsigned long lastSensorMs=0, lastWeatherMs=0, lastGpsTryMs=0, lastNtpMs=0;

// Buzzer
static void beepConnected() {
  ledcWriteTone(LEDC_BUZ_CH, 2000);
  delay(80);
  ledcWriteTone(LEDC_BUZ_CH, 2400);
  delay(80);
  ledcWriteTone(LEDC_BUZ_CH, 0);
}

// LED RGB
static void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  digitalWrite(PIN_LED_R, r>0);
  digitalWrite(PIN_LED_G, g>0);
  digitalWrite(PIN_LED_B, b>0);
}

// WiFi barres
static uint8_t wifiBars() {
  if (WiFi.status()!=WL_CONNECTED) return 0;
  int rssi = WiFi.RSSI();
  if (rssi>-55) return 4;
  if (rssi>-65) return 3;
  if (rssi>-75) return 2;
  if (rssi>-85) return 1;
  return 0;
}

// Barre d’état
static void drawStatusBar() {
  tft.fillRect(0,0,TFT_WIDTH,20,0x0000);
  drawWifiIcon(tft, 2, 1, wifiBars(), WiFi.status()!=WL_CONNECTED);
  drawWifiIcon(tft, 32, 1, wifiBars(), WiFi.status()!=WL_CONNECTED);

  String tPrev = isnan(gWeather.now.tempNow) ? "--.-" : String(gWeather.now.tempNow,1);
  String tInt  = isnan(gTempInt) ? "--.-" : String(gTempInt,1);
  String line = "Prev " + tPrev + "°  Int " + tInt + "°";
  tft.setCursor(70,4);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.print(line);

  drawWeatherIcon(tft, TFT_WIDTH-26, 0, weatherCodeToIcon(gWeather.now.conditionCode));
}

// Pages (comme avant, inchangées)
static void drawPageHome() { /* ... */ }
static void drawPageForecast() { /* ... */ }
static void drawPageAlert() { /* ... */ }
static void drawPageSensors() { /* ... */ }
static void drawPageSystem() { /* ... */ }

static void updateBacklightAndRgbByLuminosity() {
  bool lowLum = false; // TODO: remplacer par BH1750/LDR
  ledcWrite(LEDC_BL_CH, lowLum ? 30 : 200);

  if (lowLum) { setRgb(0,0,0); return; }
  if (!gWeather.now.hasAlert) { setRgb(0,255,0); return; }
  if (gWeather.now.alertSeverity=="yellow") setRgb(255,255,0);
  else if (gWeather.now.alertSeverity=="orange") setRgb(255,140,0);
  else if (gWeather.now.alertSeverity=="red") setRgb(255,0,0);
  else setRgb(0,255,0);
}

static void renderPage() {
  drawStatusBar();
  switch (currentPage) {
    case PAGE_HOME: drawPageHome(); break;
    case PAGE_FORECAST: drawPageForecast(); break;
    case PAGE_ALERT: drawPageAlert(); break;
    case PAGE_SENSORS: drawPageSensors(); break;
    case PAGE_SYSTEM: drawPageSystem(); break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BTN1, INPUT);
  pinMode(PIN_BTN2, INPUT);

  ledcSetup(LEDC_BL_CH, LEDC_BL_FREQ, LEDC_BL_RES);
  ledcAttachPin(PIN_TFT_BL, LEDC_BL_CH);
  ledcSetup(LEDC_BUZ_CH, LEDC_BUZ_FREQ, LEDC_BUZ_RES);
  ledcAttachPin(PIN_BUZZER, LEDC_BUZ_CH);

  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(0);
  tft.fillScreen(0x0000);

  Wire.begin(I2C_SDA, I2C_SCL);
  dht.begin();

  wifiMulti.addAP(WIFI_SSID1, WIFI_PASS1);
  wifiMulti.addAP(WIFI_SSID2, WIFI_PASS2);

  for (int i=0;i<15;i++) {
    wifiMulti.run();
    if (WiFi.status()==WL_CONNECTED) break;
    delay(400);
  }
  if (WiFi.status()==WL_CONNECTED) beepConnected();

  configTzTime(TZ_STRING, NTP_SERVER);

  gpsBegin(); // TinyGPS++ init

  telegramSend("Demarrage station.\n" + formatWeatherBrief());

  renderPage();
}

void loop() {
  // Boutons navigation
  static int lastBtn1=HIGH, lastBtn2=HIGH;
  int b1 = digitalRead(PIN_BTN1);
  int b2 = digitalRead(PIN_BTN2);
  if (b1==LOW && lastBtn1==HIGH) {
    currentPage = (Page)((currentPage + 1) % 5);
    renderPage();
  }
  if (b2==LOW && lastBtn2==HIGH) {
    currentPage = (Page)((currentPage + 4) % 5);
    renderPage();
  }
  lastBtn1 = b1; lastBtn2 = b2;

  // GPS loop (non bloquant)
  static GpsFix fix{false, DEFAULT_LAT, DEFAULT_LON, 0, false};
  gpsLoop(fix);
  if (fix.hasFix) {
    gLat = fix.lat;
    gLon = fix.lon;
    gUseDefaultGeo = false;
  }

  // Capteurs intérieurs
  if (millis() - lastSensorMs > REFRESH_SENSOR_MS) {
    lastSensorMs = millis();
    gTempInt = dht.readTemperature();
    gHumInt = dht.readHumidity();

    if (WiFi.status()==WL_CONNECTED) {
      if (!isnan(gTempInt) && gTempInt >= TEMP_HIGH_ALERT)
        telegramSend("Alerte: Temperature interieure elevee (" + String(gTempInt,1) + "°C)");
      if (!isnan(gTempInt) && gTempInt <= TEMP_LOW_ALERT)
        telegramSend("Alerte: Temperature interieure basse (" + String(gTempInt,1) + "°C)");
    }
    updateBacklightAndRgbByLuminosity();
    renderPage();
  }

  // Météo
  if (millis() - lastWeatherMs > REFRESH_WEATHER_MS) {
    lastWeatherMs = millis();
    if (fetchWeatherOpenWeather(gLat, gLon, gWeather)) {
      if (gWeather.now.hasAlert) {
        telegramSend("Alerte meteo: " + gWeather.now.alertTitle + "\n" + gWeather.now.alertDesc);
      }
      renderPage();
    }
  }

  // NTP resync
  if (millis() - lastNtpMs > NTP_RESYNC_MS) {
    lastNtpMs = millis();
    configTzTime(TZ_STRING, NTP_SERVER);
  }

  // Telegram commandes
  telegramLoop();

  delay(10);
}
