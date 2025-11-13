#pragma once
#include <Arduino.h>
#include <vector>

// Structure pour une prévision journalière
struct Forecast {
    float tempDay;
    float tempNight;
    int conditionCode;
};

// Structure pour la météo actuelle
struct CurrentWeather {
    float tempNow;
    int conditionCode;
    float humidity;
    float wind;
    float tempMin;
    float tempMax;
    bool hasAlert;
    String alertTitle;
    String alertDesc;
    String alertSeverity;
};

// Structure globale météo
struct WeatherData {
    CurrentWeather now;
    std::vector<Forecast> forecast;
};

String formatWeatherBrief(const WeatherData &data);

// Convertit un code OpenWeather (int) en code d'icône (String)
String weatherCodeToIcon(int code);

// Fonction de récupération météo
bool fetchWeatherOpenWeather(float lat, float lon, WeatherData &out);