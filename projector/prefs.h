#include <map>
#include <nvs.h>
#include "HomeSpan.h"

extern nvs_handle_t nvs;

struct Settings {
  static const String steps;
  static const String speed;
  static const String acs_weight;
  static const String acs_on_atv;
  static const String acs_off_atv;
};

const String Settings::steps = "Stepper: Steps per Revolution";
const String Settings::speed = "Stepper: Motor Speed";
const String Settings::acs_weight = "ACS: Low Pass weight";
const String Settings::acs_on_atv = "ACS: On Threshold Apple TV";
const String Settings::acs_off_atv = "ACS: Off Threshold Apple TV";

std::map<String, float> params = {
  {Settings::steps, 200},
  {Settings::speed, 120},
  {Settings::acs_weight, 0.01},
  {Settings::acs_on_atv, 83},
  {Settings::acs_off_atv, 76},
};

void set(String key, float val) {
  if (params.find(key) == params.end()) {
    return;
  }
  params[key] = val;
  if (nvs_set_blob(nvs, key.c_str(), &val, sizeof(float)) != ESP_OK) {
    WEBLOG("Error occurred while writing to NVS");
    Serial.println("Error occurred while writing to NVS");
    return;
  }
  if (nvs_commit(nvs) != ESP_OK) {
    WEBLOG("Couldn't commit to NVS");
    Serial.println("Couldn't commit to NVS");
  }
}

void loadSettings() {
  float value;
  size_t size = sizeof(float);
  for (const auto& [key, _]: params) {
    esp_err_t err = nvs_get_blob(nvs, key.c_str(), &value, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
      continue;
    }
    if (err != ESP_OK) {
      WEBLOG("Error occurred while reading setting from NVS");
      Serial.println("Error occurred while reading setting from NVS");
      continue;
    }
    params[key] = value;
  }
}