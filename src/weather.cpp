#include "weather.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

bool fetchWeatherOpenWeather(float lat, float lon, WeatherData &out) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi non connecte, impossible de recuperer la meteo.");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure(); // pas de vérification du certificat
    if (!client.connect("api.openweathermap.org", 443)) {
        Serial.println("Connexion a OpenWeather echouee.");
        return false;
    }

    String url = "/data/2.5/onecall?lat=" + String(lat, 6) +
                 "&lon=" + String(lon, 6) +
                 "&units=metric&lang=fr&appid=" + String(TOKEN_OPENWEATHER);

    client.println("GET " + url + " HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();

    // Lire la réponse HTTP
    String payload;
    while (client.connected() || client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") break; // fin des headers
    }
    while (client.available()) {
        payload += client.readString();
    }

    if (payload.isEmpty()) {
        Serial.println("Reponse vide de OpenWeather.");
        return false;
    }

    // Parsing JSON
    DynamicJsonDocument doc(32 * 1024); // allocation dynamique
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.print("Erreur JSON: ");
        Serial.println(err.c_str());
        return false;
    }

    // Données actuelles
    out.now.tempNow = doc["current"]["temp"].as<float>();
    out.now.conditionCode = doc["current"]["weather"][0]["id"].as<int>();
    out.now.hasAlert = false;

    // Gestion des alertes météo
    if (!doc["alerts"].isNull()) {
        JsonArray alerts = doc["alerts"].as<JsonArray>();
        if (!alerts.isNull() && alerts.size() > 0) {
            JsonObject a0 = alerts[0];
            out.now.hasAlert = true;
            out.now.alertTitle = !a0["event"].isNull() ? a0["event"].as<String>() : "Alerte météo";
            out.now.alertDesc  = !a0["description"].isNull() ? a0["description"].as<String>() : "Voir détails";
            out.now.alertSeverity = !a0["severity"].isNull() ? a0["severity"].as<String>() : "unknown";
        }
    }

    // Prévisions (exemple sur 3 jours)
    out.forecast.clear();
    if (!doc["daily"].isNull()) {
        JsonArray daily = doc["daily"].as<JsonArray>();
        for (int i = 0; i < 3 && i < daily.size(); i++) {
            JsonObject d = daily[i];
            Forecast f;
            f.tempDay = d["temp"]["day"].as<float>();
            f.tempNight = d["temp"]["night"].as<float>();
            f.conditionCode = d["weather"][0]["id"].as<int>();
            out.forecast.push_back(f);
        }
    }

    Serial.println("Meteo mise a jour depuis OpenWeather.");
    return true;
}

// Génère un résumé météo court (texte)
String formatWeatherBrief(const WeatherData &data) {
    String msg;

    // Météo actuelle
    msg += "🌡️ Temp actuelle: ";
    msg += isnan(data.now.tempNow) ? "--.-" : String(data.now.tempNow, 1);
    msg += "°C\n";

    msg += "⛅ Condition: ";
    msg += String(data.now.conditionCode);
    msg += "\n";

    // Alerte météo
    if (data.now.hasAlert) {
        msg += "⚠️ " + data.now.alertTitle + "\n";
        msg += data.now.alertDesc + "\n";
        msg += "Niveau: " + data.now.alertSeverity + "\n";
    }

    // Prévisions
    if (!data.forecast.empty()) {
        msg += "📅 Prévisions:\n";
        for (size_t i = 0; i < data.forecast.size(); i++) {
            const Forecast &f = data.forecast[i];
            msg += "Jour " + String(i+1) + ": ";
            msg += String(f.tempDay, 1) + "°C / ";
            msg += String(f.tempNight, 1) + "°C, code ";
            msg += String(f.conditionCode) + "\n";
        }
    }

    return msg;
}
