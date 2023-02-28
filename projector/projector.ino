#include <Stepper.h>
#include "ACS712.h"
#include "prefs.h"
#include "ESPAsyncWebSrv.h"
#include "LittleFS.h"
#include "AsyncJson.h"
#include "HomeSpan.h"
#include "homespan_devices.h"

#define STEPPER1 13
#define STEPPER2 27
#define STEPPER3 26
#define STEPPER4 25

// Only use pins from the first ADC channel for ACS sensors, the second is occupied by WIFI
#define ACS_1 34
#define ACS_2 35

const int ON_SWITCH_PIN = 32;

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

float mA_value_ATV  = 0;
float mA_value_PROJECTOR = 0;

Stepper motor(params[Settings::steps], STEPPER1, STEPPER2, STEPPER3, STEPPER4);
ACS712 ACS_ATV(ACS_1, 5.0, 4095, 185);
ACS712 ACS_PROJECTOR(ACS_2, 5.0, 4095, 185);
AsyncWebServer server(80);

AppleTV* appleTV;
Projector* projector;

void handleSettings(AsyncWebServerRequest *request) {
  String url = request->url();
  if (url == "/settings") {
    AsyncResponseStream * response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc(256);
    JsonArray obj = doc.to<JsonArray>();
    for (auto const& item: params) {
      obj.add(item.first);
    }
    serializeJson(doc, *response);
    request->send(response);
    return;
  }
  url.replace("/settings", "");
  url.replace("/", "");
  if (params.find(url) != params.end()) {
    if (request->method() == WebRequestMethod::HTTP_GET) {
      request->send(200, "application/json", String(params[url]));
      return;
    } else if (request->method() == WebRequestMethod::HTTP_POST) {
      if (request->hasArg("value")) {
        float val = request->arg("value").toFloat();
        Serial.printf("Update request with %s and %f\n", url.c_str(), val);
        updateSetting(url, val);
        request->send(200);
        return;
      }
    }
  }
  request->send(400);
}

void setupWeb() {
  server.on("/settings", handleSettings);
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.begin();
}


void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) {
		WEBLOG("An Error has occurred while mounting FS");
	}
  loadSettings();
  // Setup pins
  pinMode(ON_SWITCH_PIN, OUTPUT);
  // Setup homespan
  homeSpan.enableOTA();
  homeSpan.enableWebLog(20, "pool.ntp.org", "CET", "logs");
  homeSpan.setHostNameSuffix(""); // use null string for suffix (rather than the HomeSpan device ID)
	homeSpan.setPortNum(8000);		// change port number for HomeSpan so we can use port 80 for the Web Server
	homeSpan.setWifiCallback(setupWeb);
	homeSpan.begin(Category::Bridges, "HomeSpan Projector");

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  new Characteristic::Name("Apple TV Monitor");
  appleTV = new AppleTV();

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  new Characteristic::Name("Projector");
  projector = new Projector();

  // Setup ACS sensors
  ACS_ATV.autoMidPoint();
  ACS_PROJECTOR.autoMidPoint();
  ACS_ATV.suppressNoise(true);
  ACS_PROJECTOR.suppressNoise(true);
  mA_value_ATV = ACS_ATV.mA_AC();  // get initial values
  mA_value_PROJECTOR = ACS_PROJECTOR.mA_AC();
}


void loop() {
  homeSpan.poll();
  if (mA_value_ATV > params[Settings::acs_on_atv]) {
    projector->state->setVal(1);
    projector->update();
  }
  if (mA_value_ATV < params[Settings::acs_off_atv]) {
    projector->state->setVal(0);
    projector->update();
  }

}