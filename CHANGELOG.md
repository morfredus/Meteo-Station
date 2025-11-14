# Changelog

Toutes les modifications notables de ce projet seront documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/lang/fr/).

## [1.0.04-dev] - 2025-11-14

### Ajouté
- Ajout des pins SPI pour l'écran TFT ST7789 (PIN_TFT_SCL=18, PIN_TFT_SDA=23)
- Écran d'accueil au démarrage affichant :
  - Logo "METEO STATION" stylisé
  - Numéro de version
  - Progression de la connexion WiFi
  - Progression de l'initialisation GPS
  - Statut de chaque étape (I2C, DHT, NTP, Telegram)
- Initialisation SPI personnalisée avec les pins du TFT
- Rétroéclairage TFT à fond au démarrage

### Modifié
- Refonte du `setup()` pour inclure l'affichage de progression
- Amélioration de la visibilité du démarrage avec messages d'état

## [1.0.03-dev] - 2025-11-14

### Modifié
- Downgrade vers le framework ESP32 stable 6.8.1 (Arduino ESP32 2.0.17)
- Correction des erreurs de compilation WiFiClientSecure

### Corrigé
- Erreur variable membre `_connected` manquante dans WiFiClientSecure
- Erreur méthode `setSocketOption()` non déclarée
- Erreur incompatibilité signature `connect()` dans HTTPClient

## [1.0.02-dev] - 2025-11-14

### Modifié
- Désactivation temporaire de la bibliothèque ESPAsyncWebServer (non utilisée)

### Corrigé
- Erreur de compilation `WiFiServer.h: No such file or directory`
- Conflit de dépendances entre WebServer et ESP32-S3

## [1.0.01-dev] - 2025-11-14

### Ajouté
- Directive `lib_ignore = ESPAsyncTCP` dans platformio.ini
- Support complet pour ArduinoJson 7

### Modifié
- Remplacement de `DynamicJsonDocument` par `JsonDocument` dans weather.cpp
- Migration vers ArduinoJson 7 (allocation mémoire automatique)

### Corrigé
- Erreurs de compilation ESPAsyncTCP sur ESP32-S3
- Avertissements de dépréciation ArduinoJson 7
- Conflits entre ESPAsyncTCP (ESP8266) et AsyncTCP (ESP32)

## [1.0.0] - Initial

### Ajouté
- Station météo complète sur ESP32-S3
- Support TFT 1.54" ST7789
- Intégration API OpenWeather
- Support GPS avec TinyGPS++
- Notifications Telegram
- Capteurs DHT22, BMP280, BME280
- Interface multi-pages (Home, Forecast, Alert, Sensors, System)
- LED RGB pour alertes météo
- Buzzer pour notifications
- Mode économie d'énergie avec backlight adaptatif
