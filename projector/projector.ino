#include <Stepper.h>
#include "ACS712.h"
#include "prefs.h"
#include "ESPAsyncWebSrv.h"
#include "LittleFS.h"
#include "AsyncJson.h"
#include "HomeSpan.h"

#define IN1 12
#define IN2 14
#define IN3 27
#define IN4 26
#define ACS_1 34

int c = 0;
float value  = 0;

Stepper motor(params[Settings::steps], IN1, IN2, IN3, IN4);
ACS712  ACS(34, 5.0, 4095, 185);
AsyncWebServer server(80);


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
  homeSpan.enableOTA();
  homeSpan.enableWebLog(20, "pool.ntp.org", "CET", "logs");
  homeSpan.setHostNameSuffix(""); // use null string for suffix (rather than the HomeSpan device ID)
	homeSpan.setPortNum(8000);		// change port number for HomeSpan so we can use port 80 for the Web Server
	homeSpan.setWifiCallback(setupWeb);
	homeSpan.begin(Category::Bridges, "HomeSpan Projector");
  loadSettings();

  ACS.autoMidPoint();
  ACS.suppressNoise(true);
  value = ACS.mA_AC();  // get initial value
}


void loop() {
  homeSpan.poll();
  // //  select sppropriate function
  // float mA = ACS.mA_AC_sampling();
  // float weight = params[Settings::acs_weight];
  // // float mA = ACS.mA_AC();
  // value += weight * (mA - value);  // low pass filtering
  // if (c % 10 == 0) {
  //   Serial.print("weight: ");
  //   Serial.print(weight);
  //   Serial.print(" value: ");
  //   Serial.print(value, 0);
  //   Serial.print(" mA: ");
  //   Serial.print(mA);
  //   Serial.println();
  
  //   c = 0;
  // }
  // c++;
  // delay(100);
}