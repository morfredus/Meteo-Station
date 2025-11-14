#pragma once

// v1.0.05-dev - Rotation 90°, pages complètes, correction WiFi, debug météo
#define DIAGNOSTIC_VERSION "1.0.05-dev"

// Vérification de la présence du fichier secrets.h
#ifndef __has_include
  #error "Votre compilateur ne supporte pas __has_include, impossible de vérifier secrets.h"
#else
  #if __has_include("secrets.h")
    #include "secrets.h"
  #else
    #error "Fichier secrets.h manquant ! Copiez secrets_example.h en secrets.h et renseignez vos clés."
  #endif
#endif

// definition des pin
// --- [NEW FEATURE] Ajout des pins SPI pour l'écran TFT ST7789 ---
#define PIN_TFT_CS 5
#define PIN_TFT_DC 19
#define PIN_TFT_RST 4
#define PIN_TFT_BL 15
#define PIN_TFT_SCL 18  // SPI SCK (Clock)
#define PIN_TFT_SDA 23  // SPI MOSI (Data)
#define TFT_ROTATION 2

#define PIN_DHT 27

#define PIN_LED_R 14
#define PIN_LED_G 13
#define PIN_LED_B 12

#define PIN_BTN1 34
#define PIN_BTN2 35

#define PIN_BUZZER 25

#define PIN_GPS_RX 16
#define PIN_GPS_TX 17
#define PIN_GPS_PPS 26

#define I2C_SDA 21
#define I2C_SCL 22

#define LEDC_BL_CH 0
#define LEDC_BL_FREQ 5000
#define LEDC_BL_RES 8

#define LEDC_BUZ_CH 1
#define LEDC_BUZ_FREQ 2000
#define LEDC_BUZ_RES 10

// Localisation par défaut (Bordeaux)
#define DEFAULT_LAT 44.8378
#define DEFAULT_LON -0.5792
#define DEFAULT_INSEE "33063"

// Seuils & paramètres
#define LUMIN_LOW_THRESHOLD 30
#define TEMP_HIGH_ALERT 35.0
#define TEMP_LOW_ALERT  -2.0

// Affichage
#define TFT_WIDTH 240
#define TFT_HEIGHT 240

// NTP
#define NTP_SERVER "pool.ntp.org"
#define TZ_STRING "CET-1CEST,M3.5.0/2,M10.5.0/3"

// Rafraîchissements (ms)
#define REFRESH_SENSOR_MS 5000
#define REFRESH_WEATHER_MS 300000
#define RETRY_GPS_MS 15000
#define NTP_RESYNC_MS 3600000
