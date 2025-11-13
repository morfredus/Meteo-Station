#pragma once

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
