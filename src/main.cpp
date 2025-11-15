// ===============================================
// Station Météo ESP32-S3
// Version: 1.0.15
// v1.0.15 - Correction du conflit de broches des boutons avec les LEDs RGB
// v1.0.11 - Réécriture de la gestion des boutons (in-file)
// v1.0.07 - Correction navigation pages, gestion rafraîchissement écran
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

enum Page : int { PAGE_HOME, PAGE_FORECAST, PAGE_ALERT, PAGE_SENSORS, PAGE_SYSTEM };
const int NUM_PAGES = 5;
Page currentPage = PAGE_HOME;

unsigned long lastSensorMs=0, lastWeatherMs=0, lastGpsTryMs=0, lastNtpMs=0;

// ====================================================================================
// --- [REWRITE] Gestion des boutons réécrite pour plus de simplicité et de fiabilité ---
// ====================================================================================
enum ButtonEvent {
  BTN_EVT_NONE,
  BTN_EVT_1_SHORT,
  BTN_EVT_2_SHORT
};

const unsigned long DEBOUNCE_DELAY = 50;

ButtonEvent getButtonEvent() {
  static unsigned long lastBtn1Press = 0;
  static unsigned long lastBtn2Press = 0;
  static int lastBtn1State = HIGH;
  static int lastBtn2State = HIGH;

  // Lecture de l'état actuel des boutons
  int btn1State = digitalRead(PIN_BTN1);
  int btn2State = digitalRead(PIN_BTN2);

  ButtonEvent event = BTN_EVT_NONE;

  // Gestion du bouton 1
  if (btn1State == LOW && lastBtn1State == HIGH && (millis() - lastBtn1Press > DEBOUNCE_DELAY)) {
    lastBtn1Press = millis();
    event = BTN_EVT_1_SHORT;
  }
  // Gestion du bouton 2
  if (btn2State == LOW && lastBtn2State == HIGH && (millis() - lastBtn2Press > DEBOUNCE_DELAY)) {
    lastBtn2Press = millis();
    event = BTN_EVT_2_SHORT;
  }

  // Mettre à jour l'état des boutons à chaque cycle pour une détection fiable
  lastBtn1State = btn1State;
  lastBtn2State = btn2State;

  return event;
}

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

  // delay(1000); // [FIX] Remplacé par une gestion non-bloquante
}

int yPos = 170; // [FIX] Variable pour la position verticale des messages de démarrage

static void updateBootProgress(const String &message, bool success = false) {
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

void renderPage() {
  drawStatusBar();
  switch (currentPage) {
    case PAGE_HOME: drawPageHome(); break;
    case PAGE_FORECAST: drawPageForecast(); break;
    case PAGE_ALERT: drawPageAlert(); break;
    case PAGE_SENSORS: drawPageSensors(); break;
    case PAGE_SYSTEM: drawPageSystem(); break;
  }
}

unsigned long bootPauseUntil = 0;

void setup() {
  Serial.begin(115200);

  // --- [NEW FEATURE] Configuration des pins et périphériques ---
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  
  ledcSetup(LEDC_BL_CH, LEDC_BL_FREQ, LEDC_BL_RES);
  ledcAttachPin(PIN_TFT_BL, LEDC_BL_CH);
  ledcWrite(LEDC_BL_CH, 255); // Rétroéclairage à fond
  ledcSetup(LEDC_BUZ_CH, LEDC_BUZ_FREQ, LEDC_BUZ_RES);
  ledcAttachPin(PIN_BUZZER, LEDC_BUZ_CH);

  // --- [FIX] Configuration des pins des boutons ---
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);

  // Initialisation SPI avec les pins personnalisés pour le TFT
  SPI.begin(PIN_TFT_SCL, -1, PIN_TFT_SDA, PIN_TFT_CS);

  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(TFT_ROTATION); // --- [FIX] Rotation 90° (pins en haut)

  // Afficher l'écran d'accueil
  showBootScreen();
  bootPauseUntil = millis() + 1000; // Pause non-bloquante
  while (millis() < bootPauseUntil) { /* attendre */ }

  updateBootProgress("Init I2C/DHT...");
  Wire.begin(I2C_SDA, I2C_SCL);
  dht.begin();
  updateBootProgress("Init I2C/DHT", true);

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
  updateBootProgress("Config NTP", true);

  updateBootProgress("Init GPS...");
  gpsBegin();
  updateBootProgress("Init GPS", true);

  if (WiFi.status()==WL_CONNECTED) {
    updateBootProgress("Envoi telegram...");
    telegramSend("Demarrage station.\n" + formatWeatherBrief());
    updateBootProgress("Envoi telegram", true);
  }

  bootPauseUntil = millis() + 1500; // Pause non-bloquante pour lire l'écran
  while (millis() < bootPauseUntil) { /* attendre */ }

  renderPage();
  updateBacklightAndRgbByLuminosity(); // Allumer l'écran et la LED immédiatement
}

void loop() {
  // --- Gestion des événements ---
  bool needsRender = false;

  // 1. Gérer les pressions de boutons
  ButtonEvent event = getButtonEvent();
  if (event != BTN_EVT_NONE) {
    if (event == BTN_EVT_1_SHORT) {
      currentPage = (Page)(((int)currentPage + 1) % NUM_PAGES);
    } else if (event == BTN_EVT_2_SHORT) {
      currentPage = (Page)(((int)currentPage - 1 + NUM_PAGES) % NUM_PAGES);
    }
    needsRender = true;
  }
  
  // 2. Gérer le GPS
  static GpsFix fix{false, DEFAULT_LAT, DEFAULT_LON, 0, false};
  gpsLoop(fix);
  if (fix.hasFix) {
    // Note: On ne redessine pas l'écran à chaque fix GPS pour éviter le clignotement
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
        telegramSend("Alerte: Temperature interieure elevee (" + String(gTempInt,1) + " C)");
      if (!isnan(gTempInt) && gTempInt <= TEMP_LOW_ALERT)
        telegramSend("Alerte: Temperature interieure basse (" + String(gTempInt,1) + " C)");
    }
    needsRender = true;
  }

  // Météo
  if (millis() - lastWeatherMs > REFRESH_WEATHER_MS) {
    lastWeatherMs = millis();
    if (fetchWeatherOpenWeather(gLat, gLon, gWeather)) {
      if (gWeather.now.hasAlert) {
        telegramSend("Alerte meteo: " + String(gWeather.now.alertTitle) + "\n" + String(gWeather.now.alertDesc));
      }
      needsRender = true;
    }
  }

  // --- Rafraîchissement de l'affichage ---
  if (needsRender) {
    renderPage();
  }

  // NTP resync
  if (millis() - lastNtpMs > NTP_RESYNC_MS) {
    lastNtpMs = millis();
    configTzTime(TZ_STRING, NTP_SERVER);
  }

  // Telegram commandes
  telegramLoop();

  // delay(10); // [FIX] Supprimé pour une réactivité maximale
}
