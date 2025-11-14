#pragma once

#include <Arduino.h>
#include "config.h"

// Déclarations anticipées (forward declarations) des variables et fonctions
// qui sont définies dans main.cpp mais utilisées ici.
enum Page : int;
extern Page currentPage;
void renderPage();

namespace Buttons {

constexpr unsigned long DEBOUNCE_DELAY = 50; // ms
unsigned long lastDebounceTime = 0;

void setup() {
    pinMode(PIN_BTN1, INPUT);
    pinMode(PIN_BTN2, INPUT);
}

void loop() {
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        static int lastBtn1State = HIGH;
        static int lastBtn2State = HIGH;
        int btn1Reading = digitalRead(PIN_BTN1);
        int btn2Reading = digitalRead(PIN_BTN2);

        if (btn1Reading == LOW && lastBtn1State == HIGH) {
            currentPage = (Page)((currentPage + 1) % 5);
            renderPage();
            lastDebounceTime = millis();
        }
        if (btn2Reading == LOW && lastBtn2State == HIGH) {
            currentPage = (Page)((currentPage + 4) % 5); // (current - 1 + 5) % 5
            renderPage();
            lastDebounceTime = millis();
        }
        lastBtn1State = btn1Reading;
        lastBtn2State = btn2Reading;
    }
}

} // namespace Buttons