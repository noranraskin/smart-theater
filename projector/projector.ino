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
#define STEPPER4 33

// Only use pins from the first ADC channel for ACS sensors, the second is occupied by WIFI
#define ACS 34

const int LUM_SENSOR = 35;
const int ON_SWITCH = 32;
const char * canvas_mac = "08:3A:F2:B7:04:DC";
SpanPoint * canvas;

AccelStepper motor(AccelStepper::FULL4WIRE, STEPPER1, STEPPER2, STEPPER3, STEPPER4);
ACS712 ACS_PROJECTOR(ACS, 5.0, 4095, 185);
AsyncWebServer server(80);
Projector* projector;
bool ATV_STATE = false;


void turnOn() {
  if (ACS_PROJECTOR.mA_AC() < params[Settings::thresh_off_atv]) {
    digitalWrite(ON_SWITCH, HIGH);
    delay(100);
    digitalWrite(ON_SWITCH, LOW);
    bool var = true;
    canvas->send(&var);
    motor.move(params[Settings::steps]);
  }
}

void turnOff() {
  if (ACS_PROJECTOR.mA_AC() > params[Settings::thresh_on_atv]) {
      digitalWrite(ON_SWITCH, HIGH);
      delay(100);
      digitalWrite(ON_SWITCH, LOW);
      delay(300);
      digitalWrite(ON_SWITCH, HIGH);
      delay(100);
      digitalWrite(ON_SWITCH, LOW);
      bool var = false;
      canvas->send(&var);
      motor.move(-params[Settings::steps]);
  }
}

void motor_task(void * pvParameters) {
  while(true) {
    motor.run();
    if (motor.distanceToGo() == 0) {
      motor.disableOutputs();
    }
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

void handleSysCommands(AsyncWebServerRequest *request) {
  if (request->hasArg("command")) {
		String command = request->arg("command");
		if (command == "Restart") {
		  request->send(200, "text/plain", "Restarting...");
      delay(500);
      ESP.restart();
    }
	} else {
		request->send(400, "text/plain", "Bad  Request");
	}
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
    request->send(200, "application/json", String(ACS_PROJECTOR.mA_AC()));
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
		request->send(200, "text/plain", "Projector turned " + request->arg("command"));
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
  server.on("/system", handleSysCommands);
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
  // Init Spanpoint
  canvas = new SpanPoint(canvas_mac, sizeof(bool), 0);
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
}


void loop() {
  homeSpan.poll();
  int ATV_LUM = analogRead(LUM_SENSOR);
  bool old_state = ATV_STATE;
  if (ATV_LUM > params[Settings::thresh_on_atv]) {
    // Apple TV is on
    ATV_STATE = true;
  }
  if (ATV_LUM < params[Settings::thresh_off_atv]) {
    // Apple TV is off
    ATV_STATE = false;
  }
  if (old_state != ATV_STATE) {
    Serial.println("Apple TV just turned " + ATV_STATE ? "on" : "off");
    if (ATV_STATE) {
      turnOn();
    } else {
      turnOff();
    }

  }
}