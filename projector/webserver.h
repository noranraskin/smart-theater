#include "ESPAsyncWebSrv.h"
#include "AsyncJson.h"
#include <AccelStepper.h>
#include <map>
#include "prefs.h"
#include "HomeSpan.h"

extern const int LUM_SENSOR;
extern AccelStepper motor;
void send_canvas_cmd(bool);
void turnOn();
void turnOff();

void handleSettings(AsyncWebServerRequest *request) {
  String url = request->url();
  if (url == "/settings") {
    AsyncResponseStream * response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc(2048);
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
        int val = request->arg("value").toInt();
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

void handleSpanPoint(AsyncWebServerRequest *request) {
  if (request->hasArg("direction")) {
		if (request->arg("direction") == "Down") {
      send_canvas_cmd(true);
		} else if (request->arg("direction") == "Up") {
      send_canvas_cmd(false);
		} 
		request->send(200, "text/plain", "Motor moved");
	} else {
		request->send(400, "text/plain", "Bad Request");
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
