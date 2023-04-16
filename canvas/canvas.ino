#include "HomeSpan.h"
#include "ESPAsyncWebSrv.h"
#include "homespan_devices.h"
#include "LittleFS.h"

#define STEREO_RELAY 12
#define MOTOR_UP 26
#define MOTOR_DOWN 27
#define HALL 13
#define ENDSTOP_UP 32
#define ENDSTOP_DOWN 33
#define STATUS_LED	25

const char * projector_mac = "C8:C9:A3:C6:B3:68";
SpanPoint * projector;
bool projector_state = false;
bool canvas_endstop_trigger = false;

AsyncWebServer server(80);
hw_timer_t *timer = NULL;
hw_timer_t *endstop_del_enable = NULL;

void stop_motor() {
	analogWrite(MOTOR_UP, 0);
	analogWrite(MOTOR_DOWN, 0);
}

void stop_motor_up() {
	if (!canvas_endstop_trigger) {
		analogWrite(MOTOR_UP, 0);
		canvas_endstop_trigger = true;
	}
}

void stop_motor_down() {
	if (!canvas_endstop_trigger) {
		analogWrite(MOTOR_DOWN, 0);
		canvas_endstop_trigger = true;
	}
}

boolean isDown() {
	return digitalRead(ENDSTOP_DOWN);
}

boolean isUP() {
	return digitalRead(ENDSTOP_UP);
}

void move_motor(int updown) {
	move_motor_t(0, 100, updown);
}

void move_motor_t(float forSeconds, int speedPercent, int updown) {
	// When updown is 1 canvas moves up, else down
	speedPercent = max(speedPercent, 0);
	speedPercent = min(speedPercent, 100);
	int speed = map(speedPercent, 0, 100, 65, 230);
	if (updown && !isUP()) {
		analogWrite(MOTOR_DOWN, 0);
		analogWrite(MOTOR_UP, speed);
	} else if (!updown && !isDown()) {
		analogWrite(MOTOR_UP, 0);
		analogWrite(MOTOR_DOWN, speed);
	} else {
		return;
	}
	timerAlarmDisable(endstop_del_enable);
	timerWrite(endstop_del_enable, 0);
	timerAlarmWrite(endstop_del_enable, 1000000, false);
	timerAlarmEnable(endstop_del_enable);
	int uSeconds = (int)(forSeconds * 500000);
	if (uSeconds > 0) {
		timerAlarmDisable(timer);
		timerWrite(timer, 0);
		timerAlarmWrite(timer, uSeconds, false);
		timerAlarmEnable(timer);
	}
}

void handleStereo(AsyncWebServerRequest *request) {
	if (request->hasArg("command")) {
		String command = request->arg("command");
		if (command == "On") {
			digitalWrite(STEREO_RELAY, LOW);
		} else if (command == "Off") {
			digitalWrite(STEREO_RELAY, HIGH);
		}
		request->send(200, "text/plain", "Stereo turned " + request->arg("command"));
	} else {
		request->send(400, "text/plain", "Bad  Request");
	}
}

void handleCanvas(AsyncWebServerRequest *request) {
	if (request->hasArg("duration") && request->hasArg("speed")) {
		if (request->arg("direction") == "Stop") {
			stop_motor();
		}
		float duration = request->arg("duration").toFloat();
		int speed = request->arg("speed").toInt();
		if (request->arg("direction") == "Up") {
			move_motor_t(duration, speed, 1);
		} else if (request->hasArg("direction") && request->arg("direction") == "Down") {
			move_motor_t(duration, speed, 0);
		} else {
			stop_motor();
		}
		request->send(200, "text/plain", "Motor moved");
	} else {
		request->send(400, "text/plain", "Bad Request");
	}
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


void setupWeb() {
	server.on("/stereo", handleStereo);
	server.on("/canvas", handleCanvas);
	server.on("/system", handleSysCommands);
	server.serveStatic("/", LittleFS, "/").setDefaultFile("canvas.html");
	server.begin();
}

void setup() {
	Serial.begin(115200);
	pinMode(STEREO_RELAY, OUTPUT);
	pinMode(MOTOR_UP, OUTPUT);
	pinMode(MOTOR_DOWN, OUTPUT);
	pinMode(ENDSTOP_UP, INPUT_PULLDOWN);
	pinMode(ENDSTOP_DOWN, INPUT_PULLDOWN);
	pinMode(HALL, INPUT_PULLUP);
	timer = timerBegin(0, 80, true);
	endstop_del_enable = timerBegin(1, 80, true);
	timerAttachInterrupt(timer, stop_motor, true);
	timerAttachInterrupt(endstop_del_enable, []() {
		canvas_endstop_trigger = false;
	}, true);
	attachInterrupt(ENDSTOP_UP, stop_motor_up, HIGH);
	attachInterrupt(ENDSTOP_DOWN, stop_motor_down, HIGH);
	attachInterrupt(HALL, stop_motor_down, FALLING);
	// Initialize SPIFFS
	if (!LittleFS.begin()) {
		WEBLOG("An Error has occurred while mounting FS");
	}
	// Init Spanpoint
	projector = new SpanPoint(projector_mac, 0 , sizeof(bool));
	// Init homespan
	homeSpan.enableOTA();
	homeSpan.setStatusPin(STATUS_LED);
	// homeSpan.setControlPin(CONTROL_BUTTON);
	homeSpan.enableWebLog(20, "pool.ntp.org", "CET", "logs");
	homeSpan.setHostNameSuffix(""); // use null string for suffix (rather than the HomeSpan device ID)
	homeSpan.setPortNum(8000);		// change port number for HomeSpan so we can use port 80 for the Web Server
	homeSpan.setWifiCallback(setupWeb);
	homeSpan.begin(Category::Bridges, "HomeSpan Stereo");

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Canvas");
	new Canvas(move_motor, isDown, isUP);

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Stereo");
	new Stereo(STEREO_RELAY);
}

void loop() {
	// Poll homespan and webserver
	homeSpan.poll();
	if (projector->get(&projector_state)) {
		move_motor(!projector_state);
		Serial.printf("Moving canvas %s\n", (projector_state ? "down" : "up"));
	}
}