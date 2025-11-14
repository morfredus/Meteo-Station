# Changelog

Toutes les modifications notables de ce projet seront documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/lang/fr/).

## [1.0.09] - 2023-10-27
### Corrigé (Fixed)
- **Erreurs de compilation (scope)** : Correction des erreurs `is not a member of 'Buttons'` pour `ButtonEvent` et les constantes `BTN_EVT_NONE`, `BTN_EVT_1_SHORT`, `BTN_EVT_2_SHORT` en retirant le préfixe `Buttons::`.
## [1.0.08] - 2023-10-27

### Corrigé (Fixed)
- **Erreurs de compilation** : Résolution des erreurs dans `main.cpp` liées à la déclaration de `currentPage` et à l'accès aux types et énumérations de la classe `Buttons` (problème de scope).

## [1.0.07] - 2023-10-27

### Corrigé (Fixed)
- **Navigation entre les pages** : La logique de changement de page via les boutons est maintenant fonctionnelle. La boucle principale (`loop`) réagit aux événements des boutons pour changer la page courante.
- **Rafraîchissement de l'affichage** : L'écran est désormais redessiné uniquement lorsqu'un changement de page ou une mise à jour de données le nécessite, assurant une interface fluide et réactive.

### Modifié (Changed)
- **Gestion des événements** : La boucle principale a été restructurée pour gérer de manière centralisée les événements (boutons, mises à jour de capteurs) et déclencher les actions correspondantes (changement de page, rafraîchissement).
- **Version du firmware** : La version a été incrémentée à `1.0.07` dans `config.h`.

## [1.0.06] - 2023-10-27

### Corrigé (Fixed)

- **Logique des boutons** : Correction de la détection des pressions de bouton qui empêchait le changement de page. Implémentation d'un anti-rebond (debounce) pour une détection fiable.
- **Parsing des données météo** : Correction de l'analyse de la réponse JSON de l'API OpenWeatherMap. Les valeurs de température, vent, etc., sont désormais correctement extraites des objets imbriqués (`main`, `wind`).
- **Affichage des données** : Les valeurs affichées sur l'écran et envoyées via Telegram sont maintenant correctes au lieu de s'afficher à `0` ou `NaN`.

### Modifié (Changed)

- **Gestion de l'affichage** : L'écran n'est effacé que lors d'un changement de page ou d'une mise à jour des données, ce qui élimine le clignotement constant.
- **Structure de la boucle principale** : La fonction `loop()` a été réorganisée pour une meilleure gestion des états (boutons, rafraîchissement écran).
- **Mise à jour de la version** : La version du firmware est passée à `1.0.06` pour refléter ces corrections.

## [1.0.05-dev] - 2025-11-14

### Ajouté
- Implémentation complète de toutes les pages :
  - **Page HOME** : Météo actuelle avec température, min/max, humidité, vent
  - **Page FORECAST** : Prévisions sur 3 jours avec températures jour/nuit
  - **Page ALERT** : Alertes météo avec titre, sévérité et description
  - **Page SENSORS** : Données capteurs locaux (DHT22, GPS)
  - **Page SYSTEM** : Informations système (version, WiFi, RAM, uptime)
- Navigation entre pages fonctionnelle via boutons BTN1/BTN2
- Logs de debug pour la récupération des données OpenWeather

### Modifié
- Rotation de l'écran à 90° (pins en haut) via `setRotation(1)`
- Barre d'état commune à toutes les pages (ligne du haut)
- Amélioration de l'affichage des températures : "Ext" et "Int" au lieu de "Prev"
- Initialisation explicite de WeatherData avec valeurs NAN par défaut

### Corrigé
- **Icône WiFi barrée** : Correction de la logique inversée (était affichée barrée quand connecté)
- Suppression de l'icône WiFi dupliquée dans la barre d'état
- Initialisation correcte des structures de données météo
- Affichage des codes erreur JSON pour faciliter le debug

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
