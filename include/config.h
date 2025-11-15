#pragma once

// v1.0.09 - Correction erreurs de compilation (scope Buttons::)
#define DIAGNOSTIC_VERSION "1.0.09"

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
// Écran TFT ST7789 (Mode 4 fils SPI)
#define PIN_TFT_CS 5     // GPIO 5 : Chip Select (CS) - SPI
#define PIN_TFT_DC 19    // GPIO 19 : Data/Command (DC)
#define PIN_TFT_RST 4    // GPIO 4 : Reset
#define PIN_TFT_BL 15    // GPIO 15 (MTDO) : Backlight (Rétroéclairage) - (Doit être HIGH au boot)
#define PIN_TFT_SCL 18   // GPIO 18 : SPI Clock (SCK)
#define PIN_TFT_SDA 23   // GPIO 23 : SPI Data (MOSI)
#define TFT_ROTATION 2   // Rotation de l'écran (0-3)
// LED RGB (Masse Commune)
#define PIN_LED_R 14     // GPIO 14 : Output pour le Rouge
#define PIN_LED_G 13     // GPIO 13 : Output pour le Vert
#define PIN_LED_B 2      // GPIO 2 : Output pour le Bleu - (Sélectionné pour le "safe boot")
// Capteur de Luminosité (ADC)
#define PIN_LIGHT_SENSOR 35
// Deux boutons
#define PIN_BTN1 34      // GPIO 34 : Input Only - (Sélectionné pour le "safe boot" à la place de GPIO 0)
#define PIN_BTN2 27      // GPIO 27 : Input
// Buzzer
#define PIN_BUZZER 25    // GPIO 25 : Output pour le Buzzer
// Moduel GPS GT-U7 (UART 2)
#define PIN_GPS_RX 16    // GPIO 16 (RX2) : Réception (vers GPS TX)
#define PIN_GPS_TX 17    // GPIO 17 (TX2) : Transmission (vers GPS RX)
#define PIN_GPS_PPS 26   // GPIO 26 : Pulse Per Second
// Capteur GY-BME280 / OLED (I2C)
#define I2C_SDA 21       // GPIO 21 : I2C Data (SDA) - Utilisé par BME280
#define I2C_SCL 22       // GPIO 22 : I2C Clock (SCL) - Utilisé par BME280

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
