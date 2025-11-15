#include "weather.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Convertit un code OpenWeather (int) en code d'icône (String)
String weatherCodeToIcon(int code) {
    if (code == 800) return "clear";
    if (code >= 801 && code <= 804) return "clouds";
    if (code >= 200 && code <= 232) return "storm";
    if (code >= 300 && code <= 321) return "rain";
    if (code >= 500 && code <= 531) return "rain";
    if (code >= 600 && code <= 622) return "snow";
    if (code >= 701 && code <= 781) return "fog";
    return "clouds"; // par défaut
}

// --- [FIX] Ajout de logs détaillés pour le débogage ---
bool fetchWeatherOpenWeather(float lat, float lon, WeatherData &out) {
    Serial.println("\n=== [METEO] Debut recuperation donnees OpenWeather ===");

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[METEO] ERREUR: WiFi non connecte");
        return false;
    }
    Serial.println("[METEO] WiFi connecte - OK");

    Serial.print("[METEO] Connexion a api.openweathermap.org:443...");
    WiFiClientSecure client;
    client.setInsecure(); // pas de vérification du certificat
    if (!client.connect("api.openweathermap.org", 443)) {
        Serial.println(" ECHEC");
        return false;
    }
    Serial.println(" OK");

    String url = "/data/2.5/onecall?lat=" + String(lat, 6) +
                 "&lon=" + String(lon, 6) +
                 "&units=metric&lang=fr&appid=" + String(TOKEN_OPENWEATHER);

    Serial.print("[METEO] URL: ");
    Serial.println(url);

    client.println("GET " + url + " HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();

    // Lire la réponse HTTP
    Serial.println("[METEO] Lecture reponse HTTP...");
    String payload;
    int httpCode = 0;
    bool headersRead = false;

    // Lire le code de statut HTTP
    while (client.connected() || client.available()) {
        String line = client.readStringUntil('\n');
        if (line.startsWith("HTTP/1.1")) {
            httpCode = line.substring(9, 12).toInt();
            Serial.print("[METEO] Code HTTP: ");
            Serial.println(httpCode);
        }
        if (line == "\r") {
            headersRead = true;
            break; // fin des headers
        }
    }

    if (!headersRead) {
        Serial.println("[METEO] ERREUR: Headers HTTP non recus");
        return false;
    }

    // Lire le corps de la réponse
    while (client.available()) {
        payload += client.readString();
    }

    if (payload.isEmpty()) {
        Serial.println("[METEO] ERREUR: Reponse vide");
        return false;
    }

    Serial.print("[METEO] Payload recu (");
    Serial.print(payload.length());
    Serial.println(" octets)");
    Serial.print("[METEO] Premiers 300 caracteres: ");
    Serial.println(payload.substring(0, min(300, (int)payload.length())));

    // Vérifier si c'est une erreur JSON
    if (payload.indexOf("\"cod\":") >= 0 && payload.indexOf("\"message\":") >= 0) {
        Serial.println("[METEO] ATTENTION: Reponse contient un message d'erreur API");
    }

    // --- [FIX] ArduinoJson 7 : utilisation de JsonDocument au lieu de DynamicJsonDocument ---
    Serial.println("[METEO] Parsing JSON...");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.print("[METEO] ERREUR JSON: ");
        Serial.println(err.c_str());
        Serial.print("[METEO] Code erreur: ");
        Serial.println((int)err.code());
        return false;
    }
    Serial.println("[METEO] Parsing JSON - OK");

    // --- [FIX] Extraction et validation des données actuelles ---
    Serial.println("[METEO] Extraction donnees actuelles...");

    if (doc["current"].isNull()) {
        Serial.println("[METEO] ERREUR: Champ 'current' absent du JSON");
        return false;
    }

    out.now.tempNow = doc["current"]["temp"].as<float>();
    out.now.conditionCode = doc["current"]["weather"][0]["id"].as<int>();
    out.now.humidity = doc["current"]["humidity"].as<float>();
    out.now.wind = doc["current"]["wind_speed"].as<float>();

    Serial.print("[METEO] Temp actuelle: ");
    Serial.print(out.now.tempNow);
    Serial.println(" C");
    Serial.print("[METEO] Condition code: ");
    Serial.println(out.now.conditionCode);
    Serial.print("[METEO] Humidite: ");
    Serial.print(out.now.humidity);
    Serial.println(" %");
    Serial.print("[METEO] Vent: ");
    Serial.print(out.now.wind);
    Serial.println(" m/s");

    // tempMin et tempMax depuis les prévisions du jour actuel (daily[0])
    if (!doc["daily"].isNull()) {
        JsonArray daily = doc["daily"].as<JsonArray>();
        if (!daily.isNull() && daily.size() > 0) {
            JsonObject today = daily[0];
            out.now.tempMin = today["temp"]["min"].as<float>();
            out.now.tempMax = today["temp"]["max"].as<float>();
            Serial.print("[METEO] Temp min/max: ");
            Serial.print(out.now.tempMin);
            Serial.print(" / ");
            Serial.print(out.now.tempMax);
            Serial.println(" C");
        }
    } else {
        Serial.println("[METEO] ATTENTION: Champ 'daily' absent");
    }

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
            Serial.print("[METEO] Alerte detectee: ");
            Serial.println(out.now.alertTitle);
        }
    } else {
        Serial.println("[METEO] Pas d'alerte meteo");
    }

    // Prévisions (exemple sur 3 jours)
    out.forecast.clear();
    if (!doc["daily"].isNull()) {
        JsonArray daily = doc["daily"].as<JsonArray>();
        Serial.print("[METEO] Nombre de previsions: ");
        Serial.println(daily.size());
        for (int i = 0; i < 3 && i < daily.size(); i++) {
            JsonObject d = daily[i];
            Forecast f;
            f.tempDay = d["temp"]["day"].as<float>();
            f.tempNight = d["temp"]["night"].as<float>();
            f.conditionCode = d["weather"][0]["id"].as<int>();
            out.forecast.push_back(f);
            Serial.print("[METEO] Jour ");
            Serial.print(i+1);
            Serial.print(": ");
            Serial.print(f.tempDay);
            Serial.print("C / ");
            Serial.print(f.tempNight);
            Serial.println("C");
        }
    }

    Serial.println("[METEO] === Meteo mise a jour avec succes ===\n");
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
