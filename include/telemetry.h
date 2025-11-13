// telemetry.h
#pragma once
#include <Arduino.h>

void telegramSend(const String &msg);
void telegramLoop(); // commandes
String formatWeatherBrief();
