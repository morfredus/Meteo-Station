# Changelog

Toutes les modifications notables apportées à ce projet seront documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.6] - 2024-05-21

### Fixed
- **Affichage :** L'écran ne reste plus noir au démarrage. Le rétroéclairage est maintenant activé à la fin de la fonction `setup()`.
- **Démarrage :** La recherche initiale du signal GPS est maintenant limitée à 15 secondes pour ne pas bloquer indéfiniment le démarrage.

### Changed
- **Code :** Ajout de conversions explicites vers le type `String` pour éliminer les avertissements de compilation.
- **Démarrage :** La boucle de connexion WiFi et d'acquisition GPS dans `setup()` est maintenant non-bloquante et s'exécute en parallèle.

## [1.0.5] - 2024-05-21

### Fixed
- **Compilation :** Correction d'erreurs de compilation dues à des incohérences de déclaration entre `main.cpp` et `buttons.h` après refactorisation.
  - Alignement du type de l'énumération `Page` (`enum Page : int`).
  - Correction du linkage de la fonction `renderPage()` (retrait de `static`).
  - Ré-ajout de la déclaration de la variable `fix` dans la fonction `loop()`.

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