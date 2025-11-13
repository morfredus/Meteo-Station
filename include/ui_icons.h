#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>

// Dessin de l’icône WiFi (4 barres), barrée si notConnected.
inline void drawWifiIcon(Adafruit_GFX &gfx, int16_t x, int16_t y, uint8_t strength, bool notConnected) {
  // strength: 0..4
  // quatre barres, largeur 4, espacement 2, hauteur progressive
  uint8_t heights[4] = {6, 10, 14, 18};
  for (int i = 0; i < 4; i++) {
    uint16_t color = (i < strength) ? 0x07E0 /*vert*/ : 0xC618 /*gris*/;
    gfx.fillRect(x + i * 6, y + (18 - heights[i]), 4, heights[i], color);
  }
  if (notConnected) {
    // barre oblique rouge
    gfx.drawLine(x, y + 2, x + 24, y + 20, 0xF800);
    gfx.drawLine(x, y + 3, x + 24, y + 21, 0xF800);
  }
}

// Icônes météo simplifiées
inline void drawWeatherIcon(Adafruit_GFX &gfx, int16_t x, int16_t y, const String &code) {
  // code: "clear","clouds","rain","storm","fog","snow"
  if (code == "clear") {
    gfx.fillCircle(x+12, y+12, 8, 0xFFE0); // soleil jaune
  } else if (code == "clouds") {
    gfx.fillRoundRect(x+2, y+8, 20, 10, 4, 0xC618);
  } else if (code == "rain") {
    gfx.fillRoundRect(x+2, y+6, 20, 10, 4, 0xC618);
    for (int i=0;i<5;i++) gfx.drawLine(x+4+i*4, y+18, x+2+i*4, y+22, 0x001F);
  } else if (code == "storm") {
    gfx.fillRoundRect(x+2, y+6, 20, 10, 4, 0xC618);
    gfx.fillTriangle(x+8,y+18,x+14,y+18,x+10,y+24,0xF800);
  } else if (code == "fog") {
    for (int i=0;i<4;i++) gfx.drawLine(x+2, y+8+i*3, x+22, y+8+i*3, 0xC618);
  } else if (code == "snow") {
    gfx.fillRoundRect(x+2, y+6, 20, 10, 4, 0xC618);
    for (int i=0;i<4;i++) gfx.drawCircle(x+5+i*4,y+20,1,0xFFFF);
  } else {
    gfx.drawRect(x+2,y+2,20,20,0xFFFF);
  }
}
