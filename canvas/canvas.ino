#include "HomeSpan.h"
#include "WebServer.h"


#define RELAY 2
#define MOTOR_UP 0
#define MOTOR_DOWN 4
#define HALL 16
#define ENDSTOP_UP 17
#define ENDSTOP_DOWN 5

WebServer server(80);
hw_timer_t *timer = NULL;
int counter = 0;

void stop_motor() {
	analogWrite(MOTOR_UP, 0);
	analogWrite(MOTOR_DOWN, 0);
}

void move_motor(int forMilSeconds, int speedPercent, int updown) {
	speedPercent = max(speedPercent, 0);
	speedPercent = min(speedPercent, 100);
	int speed = speedPercent * 255 / 100;
	if (updown) {
		analogWrite(MOTOR_UP, speed);
	} else {
		analogWrite(MOTOR_DOWN, speed);
	}
	if (forMilSeconds > 0) {
		timerWrite(timer, 0);
		timerAlarmWrite(timer, forMilSeconds, false);
		timerAlarmEnable(timer);
	}
}

void handleRoot() {
	String html = "<html><body>";
	html += "<form action='/stereo' method='post'>";
	html += "Stereo:<br>";
	html += "<input type='submit' name='command' value='On'>";
	html += "<input type='submit' name='command' value='Off'>";
	html += "</form>";
	html += "<form action='/canvas' method='post'>";
	html += "Duration (ms): <input type='number' name='duration' value='1000'><br>";
	html += "Speed (%): <input type='number' name='speed' value='100'><br>";
	html += "<input type='submit' name='direction' value='Up'>";
	html += "<input type='submit' name='direction' value='Down'>";
	html += "<input type='submit' name='direction' value='Stop'>";
	html += "</form>";
	html += "</body></html>";
	server.send(200, "text/html", html);
}

void handleStereo() {
	if (server.hasArg("command")) {
		if (server.arg("command") == "On") {
			digitalWrite(RELAY, HIGH);
		} else if (server.arg("command") == "Off") {
			digitalWrite(RELAY, LOW);
		}
		server.send(200, "text/plain", "Stereo turned " + server.arg("command"));
	} else {
		server.send(400, "text/plain", "Bad  Request");
	}
}

void handleCanvas() {
	if (server.hasArg("duration") && server.hasArg("speed")) {
	if (server.hasArg("direction") && server.arg("direction") == "Stop") {
		stop_motor();
	}
	int duration = server.arg("duration").toInt();
	int speed = server.arg("speed").toInt();

	if (server.hasArg("direction") && server.arg("direction") == "Up") {
		move_motor(duration, speed, 1);
	} else if (server.hasArg("direction") && server.arg("direction") == "Down") {
		move_motor(duration, speed, 0);
	} else {
		stop_motor();
	}

	server.send(200, "text/plain", "Motor moved");
	} else {
	server.send(400, "text/plain", "Bad Request");
	}
}

void setupWeb() {
	server.begin();
	server.on("/", handleRoot);
	server.on("/stereo", handleStereo);
	server.on("/canvas", handleCanvas);
}

void setup() {
	Serial.begin(115200);
	pinMode(RELAY, OUTPUT);
	pinMode(MOTOR_UP, OUTPUT);
	pinMode(MOTOR_DOWN, OUTPUT);
	pinMode(ENDSTOP_UP, INPUT_PULLDOWN);
	pinMode(ENDSTOP_DOWN, INPUT_PULLDOWN);
	pinMode(HALL, INPUT_PULLUP);
	timer = timerBegin(0, 80000, true);
	timerAttachInterrupt(timer, stop_motor, true);
	attachInterrupt(ENDSTOP_UP, stop_motor, RISING); 
	attachInterrupt(ENDSTOP_DOWN, stop_motor, RISING);
	attachInterrupt(HALL, stop_motor, FALLING);
	// Init homespan
	homeSpan.setHostNameSuffix("");         // use null string for suffix (rather than the HomeSpan device ID)
	homeSpan.setPortNum(8000);              // change port number for HomeSpan so we can use port 80 for the Web Server
	homeSpan.setWifiCallback(setupWeb);
	homeSpan.begin(Category::Bridges, "HomeSpan Stereo");

	new SpanAccessory();  
    new Service::AccessoryInformation();
      new Characteristic::Identify();
}

void loop() {
	// Poll homespan and webserver
	homeSpan.poll();
	server.handleClient();
	// counter++;
	// if (counter > 1000) {
	// 	Serial.printf("Values for sensors ENDSTOP UP: %d, ENDSTOP DOWN: %d, HALL: %d\n", digitalRead(ENDSTOP_UP), digitalRead(ENDSTOP_DOWN), digitalRead(HALL));
	// 	counter = 0;
	// }
}