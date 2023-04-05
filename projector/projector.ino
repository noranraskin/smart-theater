// #include <AccelStepper.h>
#include "ACS712.h"
#include "prefs.h"
#include "ESPAsyncWebSrv.h"
#include "HTTPClient.h"
#include "LittleFS.h"
#include "HomeSpan.h"
#include "homespan_devices.h"
#include "webserver.h"
#define STEPPER1 14
#define STEPPER2 27
#define STEPPER3 26
#define STEPPER4 33

// Only use pins from the first ADC channel for ACS sensors, the second is occupied by WIFI
#define ACS 34

const int ENDSTOP = 13;
const int LUM_SENSOR = 35;
const int ON_SWITCH = 32;
const char * canvas_mac = "08:3A:F2:B7:04:DC";
SpanPoint * canvas;

AccelStepper motor(AccelStepper::FULL4WIRE, STEPPER1, STEPPER2, STEPPER3, STEPPER4);
ACS712 ACS_PROJECTOR(ACS, 5.0, 4095, 185);
AsyncWebServer server(80);
Projector* projector;
bool atv_state = false;
unsigned long last_atv_check_time = 0;
bool atv_action = false;
bool desired_projector_state = false;
unsigned long last_projector_check_time = 0;
unsigned long last_action_time = 0;

void send_post_request_to_canvas(bool var) {
  HTTPClient http;
  http.begin("http://10.10.20.157/canvas");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData;
  if (var) {
    postData = "duration=0&speed=100&direction=Down";
  } else {
    postData = "duration=0&speed=100&direction=Up";
  }
  int httpResponseCode = http.POST(postData);
  http.end();
}

void send_canvas_cmd(bool var) {
  Serial.println("Sending canvas cmd");
  if (canvas) {
    canvas->send(&var);
  } else {
    send_post_request_to_canvas(var);
  }
}

void send_canvas_cmd2(bool var) {
  Serial.println("Sending canvas cmd2");
  if (params[Settings::spanpoint_en] == 1) {
    send_canvas_cmd(var);
  }
}

void turnProjectorOn(bool var) {
  if (var) {
    digitalWrite(ON_SWITCH, HIGH);
    delay(100);
    digitalWrite(ON_SWITCH, LOW);
  } else {
    digitalWrite(ON_SWITCH, HIGH);
    delay(100);
    digitalWrite(ON_SWITCH, LOW);
    delay(300);
    digitalWrite(ON_SWITCH, HIGH);
    delay(100);
    digitalWrite(ON_SWITCH, LOW);
  }
}

void turnOn() {
    Serial.println("Turning on");
    desired_projector_state = true;
    send_canvas_cmd2(true);
    motor.moveTo(params[Settings::steps]);
}

void turnOff() {
    Serial.println("Turning off");
    desired_projector_state = false;
    send_canvas_cmd2(false);
    motor.moveTo(0);
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


void setupWeb() {
  server.on("/system", handleSysCommands);
  server.on("/sensor", handleSensor);
  server.on("/settings", handleSettings);
  server.on("/projector", handleProjector);
  server.on("/canvas", handleSpanPoint);
  server.on("/stepper", handleStepper);
  server.serveStatic("/", LittleFS, "/").setDefaultFile("projector.html");
  server.begin();
}


void setup() {
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.begin(115200);
  if (!LittleFS.begin()) {
		WEBLOG("An Error has occurred while mounting FS");
	}
  loadSettings();
  // Setup pins
  pinMode(ENDSTOP, INPUT);
  pinMode(ON_SWITCH, OUTPUT);
  pinMode(LUM_SENSOR, INPUT);
  motor.setMaxSpeed(params[Settings::speed]);
  motor.setAcceleration(params[Settings::acceleration]);
  motor.setCurrentPosition(0);
  attachInterrupt(ENDSTOP, []{
    motor.stop();
    motor.setCurrentPosition(0);
  }, RISING);
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
  // canvas = new SpanPoint(canvas_mac, 1, 0);
  // Setup homespan
  homeSpan.enableOTA();
  homeSpan.enableWebLog(20, "pool.ntp.org", "CET", "logs");
  homeSpan.setHostNameSuffix(""); // use null string for suffix (rather than the HomeSpan device ID)
	homeSpan.setPortNum(8000);		// change port number for HomeSpan so we can use port 80 for the Web Server
	homeSpan.setWifiCallback(setupWeb);
	homeSpan.begin(Category::Bridges, "HomeSpan Projector");

  // new SpanAccessory();
  // new Service::AccessoryInformation();
  // new Characteristic::Identify();

  // new SpanAccessory();
  // new Service::AccessoryInformation();
  // new Characteristic::Identify();
  // new Characteristic::Name("Projector");
  // projector = new Projector();
  // Setup ACS sensors
  ACS_PROJECTOR.autoMidPoint();
  ACS_PROJECTOR.suppressNoise(true);
}


void loop() {
  homeSpan.poll();
  int ATV_LUM = analogRead(LUM_SENSOR);
  bool old_state = atv_state;
  if (ATV_LUM > params[Settings::thresh_on_atv]) {
    // Apple TV is on
    atv_state = true;
  }
  if (ATV_LUM < params[Settings::thresh_off_atv]) {
    // Apple TV is off
    atv_state = false;
  }
  if (old_state != atv_state) {
    last_atv_check_time = millis();
    atv_action = true;
  }
  if (atv_action && (millis() - last_atv_check_time) > params[Settings::timeout_atv]) {
    Serial.printf("Apple TV just turned %s\n", (atv_state ? "on" : "off"));
    if (params[Settings::atv_led_en] == 1) {
      atv_action = false;
      if (atv_state) {
        turnOn();
      } else {
        turnOff();
      }
    }
  }
  bool actual_projector_state = desired_projector_state;
  int projector_mA = (int) ACS_PROJECTOR.mA_AC();
  if (projector_mA > params[Settings::projector_on_thresh]) {
    // Projector is on
    actual_projector_state = true;
  }
  if (projector_mA < params[Settings::projector_off_thresh]) {
    // Projector is off
    actual_projector_state = false;
  }
  if (desired_projector_state == actual_projector_state) {
    last_projector_check_time = millis();
  }
  if ((millis() - last_projector_check_time) > params[Settings::timeout_projector]) {
    if ((millis() - last_action_time) > params[Settings::projector_action_timeout]) {
      last_action_time = millis();
      Serial.printf("Turning projector %s\n", (desired_projector_state ? "on" : "off"));
      turnProjectorOn(desired_projector_state);
    }
  }
}