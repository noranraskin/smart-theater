#include <WiFi.h>
#include <HTTPClient.h>
#include <ACS712.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "helpers.h"

// ACS712 5A  uses 185 mV per A
// ACS712 20A uses 100 mV per A
// ACS712 30A uses  66 mV per A

// PIN 25
// Input Voltage 5.0 V
// Bit depth 4095
// mV per A 185 cause I use ACS712 5A
//
// Only use pins from the first ADC channel, the second is occupied by WIFI
ACS712  ACS(34, 5.0, 4095, 185);
// If current reading is more than that it will say projector is on.
int mASwitch = 2000;

const char* HOSTNAME = "Projector";
const char* CANVAS = "Canvas";
const int ON_SWITCH_PIN = 32;

enum State {
  off,
  on
};

State state = off;

// For HTTP requests
AsyncWebServer server(80);

void setup() {
  Serial.begin(9600);
  ACS.autoMidPoint();
  pinMode(ON_SWITCH_PIN, OUTPUT);
  digitalWrite(ON_SWITCH_PIN, LOW);

  connectToWifi(HOSTNAME);

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("state")) {
      AsyncWebParameter* p = request->getParam("state");
      String val = p->value();
      val.toLowerCase();
      if (val == "off" || val == "0" || val == "disabled") {
        if (state == off) {
          request->send(400, "text/plain", "Already off");
          return;
        }
        turnOff();
        request->send(200, "text/plain", "Turning off.");
        return;
      }
      if (val == "on" || val == "1" || val == "enabled") {
        if (state == on) {
          request->send(400, "text/plain", "Already on.");
          return;
        }
        turnOn();
        request->send(200, "text/plain", "Turning on.");
      }
    }
  });

  server.on("/set", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("switch")) {
      AsyncWebParameter* p = request->getParam("switch");
      int newSwitch = p->value().toInt();
      if (newSwitch == 0 && p->value() != "0") {
        request->send(400, "text/plain", "Coulnd't decode value.");
      }
      mASwitch = newSwitch;
    }
  });

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request) {
    String out;
    if (state == on) {
      out = "Projector is turned on";
    } else {
      out = "Projector is off";
    }
    request->send(200, "text/plain", out);
  });

  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ACS.mA_AC()));
  });

  server.begin();
}

void loop() {
  delay(100);
  int s_read = ACS.mA_AC();
  // Serial.println(s_read);
  if (s_read < mASwitch) {
    // off
    if (state == off) {
      return;
    } else {
      state = off;
    }
  }
  if (s_read > mASwitch) {
    // on
    if (state == on) {
      return;
    } else {
      state = on;
    }
  }
  String request = "http://";
  request += CANVAS;
  request += "/?state=";
  if (state == on) {
    request += "down";
  } else {
    request += "up";
  }

  WiFiClient client;
  HTTPClient http;
  for (int i = 0; i < 5; i++) {
    http.begin(client, request);
    int rc = http.POST("");
    if (rc == 200 || rc == 300) {
      break;
    }
    delay(500);
  }
  Serial.println("Canvas updated");
}

void turnOn() {
  digitalWrite(ON_SWITCH_PIN, HIGH);
  delay(100);
  digitalWrite(ON_SWITCH_PIN, LOW);
}

void turnOff() {
  turnOn();
  delay(300);
  turnOn();
}
