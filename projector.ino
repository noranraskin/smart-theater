#include <WiFi.h>
#include <HTTPClient.h>
#include <ACS712.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "CREDENTIALS.h"
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
const int mASwitch = 2000;

const char* HOSTNAME = "Projector";
const char* CANVAS = "Canvas";
const int ON_SWITCH_PIN = 32;

// For HTTP request
String header;
const int httpPort = 80;
WiFiServer server(httpPort);
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
int state = 0;
TaskHandle_t sensor;

void setup() {
  Serial.begin(9600);
  ACS.autoMidPoint();
  WiFi.setHostname(HOSTNAME);
  pinMode(ON_SWITCH_PIN, OUTPUT);
  digitalWrite(ON_SWITCH_PIN, LOW);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected!");
   server.begin();
  // Do the sensor reading on the second core
  xTaskCreatePinnedToCore(
    sensorloop,
    "sensor",
    10000,
    NULL,
    100,
    &sensor,
    1);
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {


            if (header.indexOf("GET /status") >= 0) {
              Serial.println("Status requested");
              String response;
              if (state == 1) {
                response = "on";
              } else {
                response = "off";
              }
              send200(client);
              client.println(response);
            } else if (header.indexOf("GET /sensor") >= 0) {
              Serial.println("Sensor data requested");
              send200(client);
              client.println(ACS.mA_AC());
            } else if (header.indexOf("PUT /?state") >= 0) {
              if (header.indexOf("state=off") >= 0 || header.indexOf("state=OFF") >= 0 || header.indexOf("state=0") >= 0) {
                if (state == 1 ) {
                  turnOff();
                  send200(client);
                  client.println("Turning off.");
                } else {
                  send400(client);
                  client.println("Already turned off.");
                }
              } else if (header.indexOf("state=on") >= 0 || header.indexOf("state=ON") >= 0 || header.indexOf("state=1") >= 0) {
                if (state == 0) {
                  turnOn();
                  send200(client);
                  client.println("Turning on");
                } else {
                  send400(client);
                  client.println("Already turned on.");
                }
              }
            }
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void sensorloop(void * parameters) {
  Serial.print("Starting sensor monitoring on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    delay(100);
    int s_read = ACS.mA_AC();
    Serial.println(s_read);
    if (s_read < mASwitch) {
      // off
      if (state == 0) {
        continue;
      } else {
        state = 0;
      }
    }
    if (s_read > mASwitch) {
      // on
      if (state == 1) {
        continue;
      } else {
        state = 1;
      }
    }
    String request = "http://";
    request += CANVAS;
    request += "/?state=";
    if (state == 1) {
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
