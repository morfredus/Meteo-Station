#include <Arduino.h>
#include <Wire.h>
#include "config.h" // Pour utiliser les définitions de broches centralisées

void setup() {
  Serial.begin(115200);
  // Attendre la connexion du moniteur série (pratique pour le débogage)
  // On ajoute un timeout pour ne pas bloquer indéfiniment si aucun moniteur n'est connecté.
  unsigned long start = millis();
  while (!Serial && (millis() - start < 2000));

  // Initialisation du bus I2C avec les broches spécifiques
  Wire.begin(I2C_SDA, I2C_SCL); 
  
  Serial.println("\n--- I2C Scanner pret ---");
  Serial.print("Utilisation des broches I2C -> SDA: ");
  Serial.print(I2C_SDA);
  Serial.print(", SCL: ");
  Serial.println(I2C_SCL);
  Serial.println("Recherche des dispositifs I2C...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("--- Nouveau Scan I2C ---");
  
  // Le bus I2C est balayé de l'adresse 1 à 127
  for(address = 1; address < 127; address++ ) {
    // La fonction beginTransmission(address) envoie la condition de START
    // et l'adresse de l'appareil (avec le bit d'écriture)
    Wire.beginTransmission(address);
    // endTransmission() renvoie 0 si l'appareil a reconnu son adresse (ACK)
    // 2: NACK sur l'adresse, 3: NACK sur la donnée, 4: autre erreur
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
      Serial.print("Erreur de bus a l'adresse 0x");
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