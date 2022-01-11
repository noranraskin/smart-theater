#include <WiFi.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "CREDENTIALS.h"

// Change these two numbers to the pins connected to your encoder.
int encoderA = 12; // Green
int encoderB = 14; // Yellow
// Change this number to the pin connected to the end stop.
int endstop = 27;
// Change these numbers to the pins connected to your relays.
// VERY IMPORTANT TO NOT GET THESE WRONG
// RISK OF SHORTING THE POWER SUPPLY
int relay1 = 5;
int relay2 = 17;
int relay3 = 16;

////////////////
int turns = 0;
int target = 0;
int up_target = 0;
// Modify this to your gear ratio and canvas size
int down_target = 16000;
// Tune the following two values to a avoid feedbackloops
int deadzone_start = 100;
int deadzone_stop = 20;
// int counter = 0; // Only for debugging
enum State {
  up,
  down,
  else_
};
State state = else_;

Preferences prefs;

const char* HOSTNAME = "Canvas";
const char* PROJECTOR = "Beamer";

// For HTTP request
AsyncWebServer server(80);

// TaskHandle_t motor;

void setup() {
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(encoderA), READ_TURN, CHANGE);
  attachInterrupt(digitalPinToInterrupt(endstop), READ_ENDSWITCH, CHANGE);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);

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

  readPrefs();

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("state")) {
      AsyncWebParameter* p = request->getParam("state");
      if (p->value() == "up") {
        if (state == up) {
          request->send(200, "text/plain", "Already up");
          return;
        }
        target = up_target;
        state = up;
        prefs.putInt("state", up);
        request->send(200, "text/plain", "Rolling up!");
        Serial.println("Rolling up!");
        return;
      } else if (p->value() == "down") {
        if (state == down) {
          request->send(200, "text/plain", "Already down");
          return;
        }
        target = down_target;
        state = down;
        prefs.putInt("state", down;)
        request->send(200, "text/plain", "Rolling down!");
        Serial.println("Rolling down!");
        return;
      } else if (p->value() == "off") {
        target = turns;
        stop_motor();
        request->send(200, "text/plain", "Stopping motor!");
        Serial.println("Stopping motor!");
        return;
      }
    }
    bool safety = true;
    if (request->hasParam("safety")) {
      AsyncWebParameter* s = request->getParam("safety");
      String safety_s = s->value();
      safety_s.toLowerCase();
      if (safety_s == "false" || safety_s == "disabled") {
        safety = false;
      }
    }
    if (request->hasParam("go-up")) {
      AsyncWebParameter* p = request->getParam("go-up");
      int amount = p->value().toInt();
      if (safety) {
        target = max(up_target, target - amount);
      } else {
        target -= amount;
      }
      state = else_;
      prefs.putInt("state", else_);
      request->send(200, "text/plain", "Rolling up to " + String(target));
      Serial.println("Going up to "+ String(turns));
      return;
    }
    if (request->hasParam("go-down")) {
      AsyncWebParameter* p = request->getParam("go-down");
      int amount = p->value().toInt();
      if (safety) {
        target = min(up_target, target - amount);
      } else {
        target += amount;
      }
      state = else_;
      prefs.putInt("state", else_);
      request->send(200, "text/plain", "Rolling down to " + String(target));
      Serial.println("Rolling down to "+ String(turns));
      return;
    }
  });

  server.on("/set", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("rolled-up-target")) {
      int old = up_target;
      AsyncWebParameter* p = request->getParam("rolled-up-target");
      String value = p->value();
      value.toLowerCase();
      if (value == "current") {
        up_target = turns;
      } else {
        int value_int = value.toInt();
        if (value_int == 0 && value != "0") {
          request->send(400, "text/plain", "Couldn't parse value");
          return;
        }
        up_target = value_int;
      }
      prefs.putInt("up_target", up_target);
      String output = "Setting rolled up target from "+ String(old) +" to "+value;
      request->send(200, "/text/plain", output);
      Serial.println(output);
    }
    if (request->hasParam("rolled-down-target")) {
      AsyncWebParameter* p = request->getParam("rolled-down-target");
      int old =  down_target;
      String value = p->value();
      value.toLowerCase();
      if (value == "current") {
        down_target = turns;
      } else {
        int value_int = value.toInt();
        if (value_int == 0 && value != "0") {
          request->send(400, "text/plain", "Couldn't parse value");
          return;
        }
        down_target = value_int;
      }
      prefs.putInt("down_target", down_target);
      String output = "Setting rolled down target from "+ String(old) +" to "+value;
      request->send(200, "/text/plain", output);
      Serial.println(output);
    }
    if (request->hasParam("target")) {
      AsyncWebParameter* p = request->getParam("target");
      int old = target;
      String value = p->value();
      int val = value.toInt();
      if (val == 0 && value != "0") {
        request->send(400, "text/plain", "Couldn't parse value");
        return;
      }
      target = val;
      String output = "Setting target from "+ String(old) +" to "+value;
      request->send(200, "/text/plain", output);
      Serial.println(output);
    }
  });

  server.on("/turns", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(turns));
  });

  server.on("/rolled-up-target", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(up_target));
  });

  server.on("/rolled-down-target", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(down_target));
  });

  server.on("/target", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(target));
  });

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request){
    String out;
    switch (state) {
    case up:
      out = "Rolled up";
      break;
    case down:
      out = "Rolled down";
      break;
    case else_:
      out = "Unknown";
      break;
    }
    request->send(200, "text/plain", out);
  });

  server.begin();
}

void loop(){
  if (target - turns > deadzone_start) {
    turn_down();
  } else if (turns - target > deadzone_start) {
    turn_up();
  }
  if (abs(turns - target) < deadzone_stop) {
    stop_motor();
  }
}

void readPrefs() {
  prefs.begin("app", false);
  up_target = prefs.getInt("up_target", up_target);
  down_target = prefs.getInt("down_target", down_target);
  state = State(prefs.getInt("state", state));
  if (state == down) {
    turns = down_target;
    target = down_target;
  } else if (state == else_) {
    turn_up();
    // rolls the canvas all the way up.
    // Only necessary if reboot during rollup/down
    while (state != up) {
      turns = 0;
    }
  }
}

void turn_up() {
  digitalWrite(relay1, LOW);
  digitalWrite(relay3, LOW);
  digitalWrite(relay2, HIGH);
}

void turn_down() {
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, HIGH);
  digitalWrite(relay1, HIGH);
}

void stop_motor() {
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
}


void READ_TURN() {
  // Interrupt function to read the x2 pulses of the encoder.
  if (digitalRead(encoderB) == 0) {
    if (digitalRead(encoderA) == 0) {
      // A fell, B is low
      turns--;
    } else {
      // A rose, B is high
      turns++;
    }
  } else {
    if (digitalRead(encoderA) == 0) {
      turns++;
    } else {
      // A rose, B is low
      turns--;
    }
  }
}

void READ_ENDSWITCH() {
  // Triggers when the canvas is up and the endstop bumps against the ceiling
  if (digitalRead(endstop) == 1) {
    stop_motor();
    up_target = turns + 50;
    state = up;
    prefs.putInt("state", up);
    prefs.putInt("up_target", up_target);
  }
}
