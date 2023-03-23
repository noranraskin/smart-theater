#include <AccelStepper.h>
#include "ACS712.h"
#include "prefs.h"
#include "ESPAsyncWebSrv.h"
#include "LittleFS.h"
#include "AsyncJson.h"
#include "HomeSpan.h"
#include "homespan_devices.h"

#define STEPPER1 14
#define STEPPER2 27
#define STEPPER3 26
#define STEPPER4 25

// Only use pins from the first ADC channel for ACS sensors, the second is occupied by WIFI
#define ACS 34

const int LUM_SENSOR = 35;
const int ON_SWITCH = 32;
int counter = 0;

// If current reading is more than that it will say projector is on.
int mASwitch = 2000;

void turnOn() {
  digitalWrite(ON_SWITCH, HIGH);
  delay(100);
  digitalWrite(ON_SWITCH, LOW);
}

void turnOff() {
  turnOn();
  delay(300);
  turnOn();
}

float mA_value_ATV  = 0;
float mA_value_PROJECTOR = 0;

AccelStepper motor(AccelStepper::FULL4WIRE, STEPPER1, STEPPER2, STEPPER3, STEPPER4);
int motor_position = 0;
bool motor_stopped = true;
ACS712 ACS_PROJECTOR(ACS, 5.0, 4095, 185);
AsyncWebServer server(80);

Projector* projector;

void motor_task(void * pvParameters) {
  for(;;) {
    motor.run();
    delay(1);
  }
}

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
        if (url == Settings::acceleration) {
          motor.setAcceleration(val);
        }
        request->send(200);
        return;
      }
    }
  }
  request->send(400);
}

void handleSensor(AsyncWebServerRequest *request) {
  String url = request->url();
  url.replace("/sensor", "");
  url.replace("/", "");
  if (url == "appleTV") {
    request->send(200, "application/json", String(analogRead(LUM_SENSOR)));
    return;
  }
  if (url == "projector") {
    request->send(200, "application/json", String(mA_value_PROJECTOR));
    return;
  }
  request->send(400, "text/plain", "Sensor not available");
  return;
}

void handleProjector(AsyncWebServerRequest *request) {
	if (request->hasArg("command")) {
		String command = request->arg("command");
		if (command == "On") {
			turnOn();
		} else if (command == "Off") {
      turnOff();
		}
		request->send(200, "text/plain", "Stereo turned " + request->arg("command"));
	} else {
		request->send(400, "text/plain", "Bad  Request");
	}
}

void handleStepper(AsyncWebServerRequest *request) {
	if (request->hasArg("steps") && request->hasArg("speed")) {
		if (request->hasArg("direction") && request->arg("direction") == "Stop") {
			motor.stop();
      motor.disableOutputs();
		}
		long steps = request->arg("steps").toInt();
		long speed = request->arg("speed").toInt();
		motor.setMaxSpeed(params[Settings::speed] * speed / 100);
		if (request->hasArg("direction") && request->arg("direction") == "Up") {
      motor.enableOutputs();
      motor.move(steps);
		} else if (request->hasArg("direction") && request->arg("direction") == "Down") {
      motor.enableOutputs();
      motor.move(-steps);
		} else {
			motor.stop();
		}
		request->send(200, "text/plain", "Motor moved");
	} else {
		request->send(400, "text/plain", "Bad Request");
	}
}

void setupWeb() {
  server.on("/sensor", handleSensor);
  server.on("/settings", handleSettings);
  server.on("/projector", handleProjector);
  server.on("/stepper", handleStepper);
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
  pinMode(ON_SWITCH, OUTPUT);
  pinMode(LUM_SENSOR, INPUT);
  motor.setMaxSpeed(params[Settings::speed]);
  motor.setAcceleration(params[Settings::acceleration]);
  motor.setCurrentPosition(0);
  xTaskCreatePinnedToCore(
    motor_task,   // Task function
    "StepperTask",   // Task name
    10000,   // Stack size
    NULL,   // Task parameters
    1,   // Task priority
    NULL,   // Task handle
    1   // Core to run the task on (0 or 1)
  );
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
  new Characteristic::Name("Projector");
  projector = new Projector();

  // Setup ACS sensors
  ACS_PROJECTOR.autoMidPoint();
  ACS_PROJECTOR.suppressNoise(true);
  mA_value_PROJECTOR = ACS_PROJECTOR.mA_AC();
}


void loop() {
  homeSpan.poll();
}