#include <Wire.h>

// Définition des broches I2C pour l'ESP32 uPesy DevKit
// SDA = GPIO 21, SCL = GPIO 22
const int SDA_PIN = 21;
const int SCL_PIN = 22;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Attendre la connexion du moniteur série (pratique pour l'ESP32)

  // Initialisation du bus I2C avec les broches spécifiques
  Wire.begin(SDA_PIN, SCL_PIN); 
  
  Serial.println("\n--- I2C Scanner pret ---");
  Serial.println("Recherche des dispositifs I2C...");
}

void loop() {
  byte error, address;
  int nDevices;

  Serial.println("--- Nouveau Scan I2C ---");
  nDevices = 0;
  
  // Le bus I2C est balayé de l'adresse 1 à 127
  for(address = 1; address < 127; address++ ) {
    // La fonction beginTransmission(address) envoie la condition de START
    // et l'adresse de l'appareil (avec le bit d'écriture)
    Wire.beginTransmission(address);
    // La fonction endTransmission() renvoie 0 si l'appareil a reconnu son adresse (ACK)
    error = Wire.endTransmission(); 

    if (error == 0) {
      Serial.print("Dispositif I2C trouve a l'adresse 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  (DEC: " + String(address) + ")");
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Erreur inconnue a l'adresse 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address, HEX);
    }    
  }
  
  if (nDevices == 0) {
    Serial.println("Aucun dispositif I2C trouve.");
  } else {
    Serial.println("Scan I2C termine.");
  }
  
  Serial.println("-------------------------");
  // Pause de 5 secondes avant le prochain scan
  delay(5000); 
}