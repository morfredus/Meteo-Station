# Changelog

Toutes les modifications notables apportées à ce projet seront documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.4] - 2024-05-21

### Added
- Création du fichier `CHANGELOG.md` pour centraliser et suivre l'historique des versions du projet.

### Changed
- L'historique des versions a été retiré de l'en-tête de `main.cpp` pour éviter la redondance.

## [1.0.3] - 2024-05-21

### Added
- Ajout d'un système d'anti-rebond logiciel (`debounce`) pour les boutons de navigation, améliorant la fiabilité des appuis.

### Changed
- **Refactorisation :** La logique de gestion des boutons a été extraite de `main.cpp` vers un fichier dédié `include/buttons.h` pour une meilleure organisation du code.

### Fixed
- **Compilation :** Résolution de multiples erreurs de compilation en figeant la version du framework `espressif32` à `6.6.0` pour garantir la compatibilité des bibliothèques (`WiFiClientSecure`, `HTTPClient`).
- **Configuration :** Correction du type de carte dans `platformio.ini` de `esp32-s3-devkitc-1` à `upesy_wroom` pour correspondre au matériel réel.
- **Dépendances :** Suppression de la bibliothèque `arduino-libraries/WiFi` conflictuelle dans `platformio.ini`.

## [Versions de développement initiales] - Avant 1.0.3

- **v1.0.01-dev à v1.0.03-dev :** Diverses tentatives de corrections de compilation liées aux bibliothèques (`AsyncTCP`, `ArduinoJson 7`) et aux versions du framework ESP32.