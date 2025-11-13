#include "config.h"
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

Adafruit_ST7789 tft = Adafruit_ST7789(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
DHT dht(PIN_DHT, DHT22); // bascule vers DHT11 si besoin

WeatherData gWeather;
float gTempInt = NAN, gHumInt = NAN;
double gLat = DEFAULT_LAT, gLon = DEFAULT_LON;
bool gUseDefaultGeo = true;

enum Page { PAGE_HOME, PAGE_FORECAST, PAGE_ALERT, PAGE_SENSORS, PAGE_SYSTEM };
Page currentPage = PAGE_HOME;

unsigned long lastSensorMs=0, lastWeatherMs=0, lastGpsTryMs=0, lastNtpMs=0;

static void beepConnected() {
  ledcWriteTone(LEDC_BUZ_CH, 2000);
  delay(80);
  ledcWriteTone(LEDC_BUZ_CH, 2400);
  delay(80);
  ledcWriteTone(LEDC_BUZ_CH, 0);
}

static void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  // Common minus, drive each channel via digitalWrite (on/off) or PWM via LEDC (optionnel)
  digitalWrite(PIN_LED_R, r>0);
  digitalWrite(PIN_LED_G, g>0);
  digitalWrite(PIN_LED_B, b>0);
}

static uint8_t wifiBars() {
  if (WiFi.status()!=WL_CONNECTED) return 0;
  int rssi = WiFi.RSSI();
  if (rssi>-55) return 4;
  if (rssi>-65) return 3;
  if (rssi>-75) return 2;
  if (rssi>-85) return 1;
  return 0;
}

static void drawStatusBar() {
  tft.fillRect(0,0,TFT_WIDTH,20,0x0000);
  // deux icônes wifi côté à côte
  drawWifiIcon(tft, 2, 1, wifiBars(), WiFi.status()!=WL_CONNECTED);
  drawWifiIcon(tft, 32, 1, wifiBars(), WiFi.status()!=WL_CONNECTED);

  // centre: T° prévision + T° DHT
  String tPrev = isnan(gWeather.now.tempNow) ? "--.-" : String(gWeather.now.tempNow,1);
  String tInt  = isnan(gTempInt) ? "--.-" : String(gTempInt,1);
  String line = "Prev " + tPrev + "°  Int " + tInt + "°";
  tft.setCursor(70,4);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.print(line);

  // icône météo à droite
  drawWeatherIcon(tft, TFT_WIDTH-26, 0, gWeather.now.conditionCode);
}

static void drawPageHome() {
  tft.fillRect(0,20,TFT_WIDTH,TFT_HEIGHT-20,0x0000);
  tft.setTextColor(0xFFFF); tft.setTextSize(1);
  // Heure (NTP)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S  %d/%m/%Y", &timeinfo);
    tft.setCursor(10, 30); tft.print("Heure: "); tft.print(buf);
  } else {
    tft.setCursor(10, 30); tft.print("Heure: sync en cours...");
  }

  // Extérieur (prévision comme proxy + humidité/vent)
  tft.setCursor(10, 50);
  tft.print("Exterieur: ");
  tft.print(isnan(gWeather.now.tempNow)?"--.-":String(gWeather.now.tempNow,1));
  tft.print("°C, hum ");
  tft.print(isnan(gWeather.now.humidity)?"--":String(gWeather.now.humidity,0));
  tft.print("%, vent ");
  tft.print(isnan(gWeather.now.wind)?"--.-":String(gWeather.now.wind,1));
  tft.print(" m/s");

  // Intérieur (DHT)
  tft.setCursor(10, 70);
  tft.print("Interieur: ");
  tft.print(isnan(gTempInt)?"--.-":String(gTempInt,1));
  tft.print("°C, ");
  tft.print(isnan(gHumInt)?"--":String(gHumInt,0));
  tft.print("%");

  // Alerte
  tft.setCursor(10, 90);
  if (gWeather.now.hasAlert) {
    tft.setTextColor(0xFFE0);
    tft.print("ALERTE: ");
    tft.print(gWeather.now.alertTitle);
  } else {
    tft.setTextColor(0x07E0);
    tft.print("Pas d’alerte.");
  }
}

static void drawPageForecast() {
  tft.fillRect(0,20,TFT_WIDTH,TFT_HEIGHT-20,0x0000);
  tft.setTextColor(0xFFFF); tft.setTextSize(1);
  tft.setCursor(10, 30); tft.print("Prévisions (4 jours):");
  for (int i=0;i<4;i++) {
    tft.setCursor(10, 50 + i*30);
    tft.print("J"); tft.print(i); tft.print(": ");
    tft.print("min "); tft.print(gWeather.daily[i].tmin,1); tft.print("° / ");
    tft.print("max "); tft.print(gWeather.daily[i].tmax,1); tft.print("°  ");
    tft.print("pluie "); tft.print(gWeather.daily[i].pop,0); tft.print("%");
    drawWeatherIcon(tft, TFT_WIDTH-26, 46 + i*30, gWeather.daily[i].code);
  }
}

static void drawPageAlert() {
  tft.fillRect(0,20,TFT_WIDTH,TFT_HEIGHT-20,0x0000);
  tft.setTextColor(0xFFFF); tft.setTextSize(1);
  tft.setCursor(10, 30);
  if (gWeather.now.hasAlert) {
    tft.print("Alerte: "); tft.print(gWeather.now.alertTitle);
    tft.setCursor(10, 50);
    tft.print(gWeather.now.alertDesc);
  } else {
    tft.print("Aucune alerte en cours.");
  }
}

static void drawPageSensors() {
  tft.fillRect(0,20,TFT_WIDTH,TFT_HEIGHT-20,0x0000);
  tft.setTextColor(0xFFFF); tft.setTextSize(1);
  tft.setCursor(10, 30); tft.print("Capteurs:");
  tft.setCursor(10, 50); tft.print("DHT: "); tft.print(gTempInt,1); tft.print("°C "); tft.print(gHumInt,0); tft.print("%");
  tft.setCursor(10, 70); tft.print("GPS fix: "); tft.print(gUseDefaultGeo?"non (def.)":"oui");
  tft.setCursor(10, 90); tft.print("Lat/Lon: "); tft.print(gLat,5); tft.print(", "); tft.print(gLon,5);
  tft.setCursor(10, 110); tft.print("WiFi RSSI: "); tft.print(WiFi.RSSI());
}

static void drawPageSystem() {
  tft.fillRect(0,20,TFT_WIDTH,TFT_HEIGHT-20,0x0000);
  tft.setTextColor(0xFFFF); tft.setTextSize(1);
  tft.setCursor(10, 30); tft.print("Systeme:");
  tft.setCursor(10, 50); tft.print("IP: "); tft.print(WiFi.localIP().toString());
  tft.setCursor(10, 70); tft.print("Uptime: "); tft.print(millis()/1000); tft.print(" s");
  tft.setCursor(10, 90); tft.print("Version: v1.0.0");
}

static void updateBacklightAndRgbByLuminosity() {
  // Si tu ajoutes BH1750, lis la valeur en lux. Ici, on suppose luminosité faible si RSSI très bas (placeholder).
  bool lowLum = false; // TODO: remplacer par lecture BH1750/LDR
  // PWM du rétroéclairage (0-255)
  ledcWrite(LEDC_BL_CH, lowLum ? 30 : 200);

  // LED RGB: verte si pas d’alerte, sinon couleur de sévérité. Éteinte la nuit/faible lumière.
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
  // Pins
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  pinMode(PIN_BTN1, INPUT); // besoin pull-up externe
  pinMode(PIN_BTN2, INPUT); // besoin pull-up externe

  // LEDC
  ledcSetup(LEDC_BL_CH, LEDC_BL_FREQ, LEDC_BL_RES);
  ledcAttachPin(PIN_TFT_BL, LEDC_BL_CH);
  ledcSetup(LEDC_BUZ_CH, LEDC_BUZ_FREQ, LEDC_BUZ_RES);
  ledcAttachPin(PIN_BUZZER, LEDC_BUZ_CH);

  // TFT
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(0);
  tft.fillScreen(0x0000);

  // I2C + DHT
  Wire.begin(I2C_SDA, I2C_SCL);
  dht.begin();

  // WiFiMulti
  wifiMulti.addAP(WIFI_SSID1, WIFI_PASS1);
  wifiMulti.addAP(WIFI_SSID2, WIFI_PASS2);

  tft.setTextColor(0xFFFF); tft.setTextSize(1);
  tft.setCursor(10, 100);
  tft.print("Connexion Wifi");
  tft.setCursor(10, 120);
  tft.print("SSID: ");
  tft.print(WIFI_SSID1);

  for (int i=0;i<15;i++) {
    wifiMulti.run();
    if (WiFi.status()==WL_CONNECTED) break;
    delay(400);
  }
  if (WiFi.status()==WL_CONNECTED) {
    beepConnected();
  } else {
    // tente le second SSID
    tft.setCursor(10, 140);
    tft.print("SSID: ");
    tft.print(WIFI_SSID2);
    for (int i=0;i<15;i++) {
      wifiMulti.run();
      if (WiFi.status()==WL_CONNECTED) break;
      delay(400);
    }
    if (WiFi.status()==WL_CONNECTED) beepConnected();
  }

  // NTP
  configTzTime(TZ_STRING, NTP_SERVER);

  // GPS
  gpsBegin();
  gpsRequestAssistNow();

  // Envoi Telegram au démarrage
  telegramSend("Demarrage station.\n" + formatWeatherBrief());

  renderPage();
}

void loop() {
  // Boutons (défilement)
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

  // GPS loop
  static GpsFix fix{false, DEFAULT_LAT, DEFAULT_LON, 0, false};
  gpsLoop(fix);

  if (fix.hasFix) {
    gLat = fix.lat;
    gLon = fix.lon;
    gUseDefaultGeo = false;
  }

  // Re-tente GPS périodiquement (non-bloquant)
  if (millis() - lastGpsTryMs > RETRY_GPS_MS) {
    lastGpsTryMs = millis();
    gpsRequestAssistNow();
  }

  // Capteurs intérieurs (DHT)
  if (millis() - lastSensorMs > REFRESH_SENSOR_MS) {
    lastSensorMs = millis();
    gTempInt = dht.readTemperature();
    gHumInt = dht.readHumidity();

    // Alerte température extrême
    if (WiFi.status()==WL_CONNECTED) {
      if (!isnan(gTempInt) && gTempInt >= TEMP_HIGH_ALERT) telegramSend("Alerte: Temperature interieure elevee (" + String(gTempInt,1) + "°C)");
      if (!isnan(gTempInt) && gTempInt <= TEMP_LOW_ALERT) telegramSend("Alerte: Temperature interieure basse (" + String(gTempInt,1) + "°C)");
    }
    updateBacklightAndRgbByLuminosity();
    renderPage();
  }

  // Météo
  if (millis() - lastWeatherMs > REFRESH_WEATHER_MS) {
    lastWeatherMs = millis();
    if (fetchWeatherOpenWeather(gLat, gLon, gWeather)) {
      // alerte
      if (gWeather.now.hasAlert) {
        telegramSend("Alerte meteo: " + gWeather.now.alertTitle + "\n" + gWeather.now.alertDesc);
      }
      renderPage();
    }
  }

  // NTP resync périodique
  if (millis() - lastNtpMs > NTP_RESYNC_MS) {
    lastNtpMs = millis();
    configTzTime(TZ_STRING, NTP_SERVER);
  }

  // Telegram commandes
  telegramLoop();

  delay(10);
}
