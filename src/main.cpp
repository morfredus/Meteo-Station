// ===============================================
// Station Météo ESP32-S3
// Version: 1.0.05-dev
// v1.0.01-dev - Correction compilation ESP32-S3 : AsyncTCP et ArduinoJson 7
// v1.0.02-dev - Désactivation ESPAsyncWebServer (non utilisé, conflit WiFiServer.h)
// v1.0.03-dev - Framework ESP32 stable 6.8.1 (corrige bugs WiFiClientSecure/HTTPClient)
// v1.0.04-dev - Ajout pins SPI TFT, écran d'accueil avec progression démarrage
// v1.0.05-dev - Rotation 90°, pages complètes, correction WiFi, debug météo
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

// --- [FIX] Initialisation explicite des données météo ---
WeatherData gWeather = {
  .now = {
    .tempNow = NAN,
    .conditionCode = 0,
    .humidity = NAN,
    .wind = NAN,
    .tempMin = NAN,
    .tempMax = NAN,
    .hasAlert = false,
    .alertTitle = "",
    .alertDesc = "",
    .alertSeverity = ""
  },
  .forecast = {}
};
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

// --- [FIX] Barre d'état corrigée (WiFi, températures, icône météo) ---
static void drawStatusBar() {
  tft.fillRect(0,0,TFT_WIDTH,20,0x0000);

  // Icône WiFi (UNE seule fois, avec logique inversée)
  bool notConnected = (WiFi.status() != WL_CONNECTED);
  drawWifiIcon(tft, 2, 1, wifiBars(), notConnected);

  // Températures
  String tPrev = isnan(gWeather.now.tempNow) ? "--.-" : String(gWeather.now.tempNow,1);
  String tInt  = isnan(gTempInt) ? "--.-" : String(gTempInt,1);
  String line = "Ext " + tPrev + "C Int " + tInt + "C";
  tft.setCursor(32,4);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.print(line);

  // Icône météo
  drawWeatherIcon(tft, TFT_WIDTH-26, 0, weatherCodeToIcon(gWeather.now.conditionCode));
}

// --- [NEW FEATURE] Écran d'accueil au démarrage ---
static void showBootScreen() {
  tft.fillScreen(0x0000); // Fond noir

  // Logo/Titre centré
  tft.setTextColor(0x07FF); // Cyan
  tft.setTextSize(3);
  int16_t x = (TFT_WIDTH - 18*8) / 2; // Centrage approximatif
  tft.setCursor(x, 60);
  tft.println("METEO");
  tft.setCursor(x-10, 95);
  tft.println("STATION");

  // Version
  tft.setTextColor(0xFFE0); // Jaune
  tft.setTextSize(1);
  tft.setCursor(70, 130);
  tft.print("Version ");
  tft.println(DIAGNOSTIC_VERSION);

  // Ligne de séparation
  tft.drawFastHLine(40, 150, TFT_WIDTH-80, 0x07FF);

  delay(1000);
}

static void updateBootProgress(const String &message, bool success = false) {
  static int yPos = 170;

  if (yPos > TFT_HEIGHT - 20) {
    // Effacer la zone de progression si on déborde
    tft.fillRect(0, 165, TFT_WIDTH, TFT_HEIGHT-165, 0x0000);
    yPos = 170;
  }

  tft.setTextSize(1);
  if (success) {
    tft.setTextColor(0x07E0); // Vert
    tft.setCursor(10, yPos);
    tft.print("[OK] ");
  } else {
    tft.setTextColor(0xFFFF); // Blanc
    tft.setCursor(10, yPos);
    tft.print("[..] ");
  }
  tft.println(message);
  yPos += 12;
}

// --- [NEW FEATURE] Implémentation complète des pages ---

static void drawPageHome() {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT-20, 0x0000);

  // Titre
  tft.setTextColor(0x07FF);
  tft.setTextSize(2);
  tft.setCursor(10, 30);
  tft.println("METEO ACTUELLE");

  // Température principale
  tft.setTextColor(0xFFE0);
  tft.setTextSize(4);
  tft.setCursor(40, 60);
  if (!isnan(gWeather.now.tempNow)) {
    tft.print(gWeather.now.tempNow, 1);
    tft.println(" C");
  } else {
    tft.println("--.-C");
  }

  // Min/Max
  tft.setTextSize(1);
  tft.setTextColor(0x07E0);
  tft.setCursor(10, 110);
  tft.print("Min:");
  if (!isnan(gWeather.now.tempMin)) tft.print(gWeather.now.tempMin,1);
  else tft.print("--.-");
  tft.print("C  Max:");
  if (!isnan(gWeather.now.tempMax)) tft.print(gWeather.now.tempMax,1);
  else tft.print("--.-");
  tft.println("C");

  // Humidité et vent
  tft.setTextColor(0xFFFF);
  tft.setCursor(10, 130);
  tft.print("Humidite: ");
  if (!isnan(gWeather.now.humidity)) tft.print(gWeather.now.humidity,0);
  else tft.print("--");
  tft.println(" %");

  tft.setCursor(10, 145);
  tft.print("Vent: ");
  if (!isnan(gWeather.now.wind)) tft.print(gWeather.now.wind,1);
  else tft.print("--.-");
  tft.println(" m/s");

  // Condition météo
  tft.setCursor(10, 165);
  tft.setTextColor(0x07FF);
  tft.print("Code: ");
  tft.println(gWeather.now.conditionCode);

  // Icône grande
  drawWeatherIcon(tft, 180, 60, weatherCodeToIcon(gWeather.now.conditionCode));

  // Navigation
  tft.setTextColor(0xC618);
  tft.setTextSize(1);
  tft.setCursor(10, TFT_HEIGHT-15);
  tft.print("BTN1:Page suiv. BTN2:Page prec.");
}

static void drawPageForecast() {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT-20, 0x0000);

  tft.setTextColor(0x07FF);
  tft.setTextSize(2);
  tft.setCursor(10, 30);
  tft.println("PREVISIONS");

  int yPos = 60;
  if (gWeather.forecast.empty()) {
    tft.setTextColor(0xF800);
    tft.setTextSize(1);
    tft.setCursor(10, yPos);
    tft.println("Aucune prevision disponible");
  } else {
    tft.setTextSize(1);
    for (size_t i = 0; i < gWeather.forecast.size() && i < 3; i++) {
      const Forecast &f = gWeather.forecast[i];

      tft.setTextColor(0xFFE0);
      tft.setCursor(10, yPos);
      tft.print("Jour ");
      tft.println(i+1);

      tft.setTextColor(0xFFFF);
      tft.setCursor(20, yPos+15);
      tft.print("Journee: ");
      tft.print(f.tempDay, 1);
      tft.println("C");

      tft.setCursor(20, yPos+30);
      tft.print("Nuit: ");
      tft.print(f.tempNight, 1);
      tft.println("C");

      tft.setCursor(20, yPos+45);
      tft.print("Code: ");
      tft.println(f.conditionCode);

      drawWeatherIcon(tft, 180, yPos+10, weatherCodeToIcon(f.conditionCode));

      yPos += 70;
    }
  }

  tft.setTextColor(0xC618);
  tft.setTextSize(1);
  tft.setCursor(10, TFT_HEIGHT-15);
  tft.print("BTN1:Page suiv. BTN2:Page prec.");
}

static void drawPageAlert() {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT-20, 0x0000);

  tft.setTextColor(0x07FF);
  tft.setTextSize(2);
  tft.setCursor(10, 30);
  tft.println("ALERTES METEO");

  if (!gWeather.now.hasAlert) {
    tft.setTextColor(0x07E0);
    tft.setTextSize(2);
    tft.setCursor(30, 100);
    tft.println("Pas d'alerte");
  } else {
    // Titre de l'alerte
    tft.setTextColor(0xF800);
    tft.setTextSize(1);
    tft.setCursor(10, 60);
    tft.println(gWeather.now.alertTitle);

    // Sévérité
    tft.setTextColor(0xFFE0);
    tft.setCursor(10, 80);
    tft.print("Niveau: ");
    tft.println(gWeather.now.alertSeverity);

    // Description (limitée pour tenir sur l'écran)
    tft.setTextColor(0xFFFF);
    tft.setCursor(10, 100);
    String desc = gWeather.now.alertDesc;
    if (desc.length() > 200) desc = desc.substring(0, 197) + "...";
    tft.println(desc);
  }

  tft.setTextColor(0xC618);
  tft.setTextSize(1);
  tft.setCursor(10, TFT_HEIGHT-15);
  tft.print("BTN1:Page suiv. BTN2:Page prec.");
}

static void drawPageSensors() {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT-20, 0x0000);

  tft.setTextColor(0x07FF);
  tft.setTextSize(2);
  tft.setCursor(10, 30);
  tft.println("CAPTEURS LOCAUX");

  // DHT22
  tft.setTextColor(0xFFE0);
  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.println("DHT22 (Interieur):");

  tft.setTextColor(0xFFFF);
  tft.setCursor(20, 75);
  tft.print("Temperature: ");
  if (!isnan(gTempInt)) tft.print(gTempInt,1);
  else tft.print("--.-");
  tft.println(" C");

  tft.setCursor(20, 90);
  tft.print("Humidite: ");
  if (!isnan(gHumInt)) tft.print(gHumInt,0);
  else tft.print("--");
  tft.println(" %");

  // GPS
  tft.setTextColor(0xFFE0);
  tft.setCursor(10, 120);
  tft.println("GPS:");

  tft.setTextColor(0xFFFF);
  tft.setCursor(20, 135);
  tft.print("Lat: ");
  tft.println(gLat, 5);

  tft.setCursor(20, 150);
  tft.print("Lon: ");
  tft.println(gLon, 5);

  tft.setCursor(20, 165);
  if (gUseDefaultGeo) {
    tft.setTextColor(0xF800);
    tft.println("(Position par defaut)");
  } else {
    tft.setTextColor(0x07E0);
    tft.println("(Position GPS)");
  }

  tft.setTextColor(0xC618);
  tft.setTextSize(1);
  tft.setCursor(10, TFT_HEIGHT-15);
  tft.print("BTN1:Page suiv. BTN2:Page prec.");
}

static void drawPageSystem() {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT-20, 0x0000);

  tft.setTextColor(0x07FF);
  tft.setTextSize(2);
  tft.setCursor(10, 30);
  tft.println("SYSTEME");

  // Version
  tft.setTextColor(0xFFE0);
  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.print("Version: ");
  tft.println(DIAGNOSTIC_VERSION);

  // WiFi
  tft.setTextColor(0xFFFF);
  tft.setCursor(10, 80);
  tft.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(0x07E0);
    tft.println("Connecte");
    tft.setTextColor(0xFFFF);
    tft.setCursor(10, 95);
    tft.print("SSID: ");
    tft.println(WiFi.SSID());
    tft.setCursor(10, 110);
    tft.print("IP: ");
    tft.println(WiFi.localIP());
    tft.setCursor(10, 125);
    tft.print("RSSI: ");
    tft.print(WiFi.RSSI());
    tft.println(" dBm");
  } else {
    tft.setTextColor(0xF800);
    tft.println("Deconnecte");
  }

  // Mémoire
  tft.setTextColor(0xFFFF);
  tft.setCursor(10, 150);
  tft.print("RAM libre: ");
  tft.print(ESP.getFreeHeap() / 1024);
  tft.println(" KB");

  // Uptime
  tft.setCursor(10, 165);
  tft.print("Uptime: ");
  unsigned long uptime = millis() / 1000;
  tft.print(uptime / 3600);
  tft.print("h ");
  tft.print((uptime % 3600) / 60);
  tft.println("m");

  tft.setTextColor(0xC618);
  tft.setTextSize(1);
  tft.setCursor(10, TFT_HEIGHT-15);
  tft.print("BTN1:Page suiv. BTN2:Page prec.");
}

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

  // --- [NEW FEATURE] Configuration des pins et périphériques ---
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BTN1, INPUT);
  pinMode(PIN_BTN2, INPUT);

  ledcSetup(LEDC_BL_CH, LEDC_BL_FREQ, LEDC_BL_RES);
  ledcAttachPin(PIN_TFT_BL, LEDC_BL_CH);
  ledcWrite(LEDC_BL_CH, 255); // Rétroéclairage à fond
  ledcSetup(LEDC_BUZ_CH, LEDC_BUZ_FREQ, LEDC_BUZ_RES);
  ledcAttachPin(PIN_BUZZER, LEDC_BUZ_CH);

  // Initialisation SPI avec les pins personnalisés pour le TFT
  SPI.begin(PIN_TFT_SCL, -1, PIN_TFT_SDA, PIN_TFT_CS);

  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(1); // --- [FIX] Rotation 90° (pins en haut)

  // Afficher l'écran d'accueil
  showBootScreen();

  updateBootProgress("Init I2C/DHT...");
  Wire.begin(I2C_SDA, I2C_SCL);
  dht.begin();
  delay(300);
  updateBootProgress("Init I2C/DHT...", true);

  updateBootProgress("Connexion WiFi...");
  wifiMulti.addAP(WIFI_SSID1, WIFI_PASS1);
  wifiMulti.addAP(WIFI_SSID2, WIFI_PASS2);

  for (int i=0;i<15;i++) {
    wifiMulti.run();
    if (WiFi.status()==WL_CONNECTED) break;
    delay(400);
  }

  if (WiFi.status()==WL_CONNECTED) {
    updateBootProgress("WiFi connecte", true);
    beepConnected();
  } else {
    updateBootProgress("WiFi echec", false);
  }

  updateBootProgress("Config NTP...");
  configTzTime(TZ_STRING, NTP_SERVER);
  delay(300);
  updateBootProgress("Config NTP...", true);

  updateBootProgress("Init GPS...");
  gpsBegin();
  delay(300);
  updateBootProgress("Init GPS...", true);

  if (WiFi.status()==WL_CONNECTED) {
    updateBootProgress("Envoi telegram...");
    telegramSend("Demarrage station.\n" + formatWeatherBrief());
    delay(300);
    updateBootProgress("Envoi telegram...", true);
  }

  delay(1500); // Pause pour lire l'écran de démarrage
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
