#include <WiFi.h>
#include "CREDENTIALS.h"

void connectToWifi(const char* HOSTNAME) {
  WiFi.setHostname(HOSTNAME);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Connected!");
}