#include <map>
#include <nvs.h>
#include <nvs_flash.h>

using std::hash;

nvs_handle_t nvs;
const char* setting_ns = "settings";

struct Settings {
  static const String steps;
  static const String speed;
  static const String acs_weight;
  static const String acs_on_atv;
  static const String acs_off_atv;
  static const String hs_update_interval;
};

const String Settings::steps = "Stepper: Steps per Revolution";
const String Settings::speed = "Stepper: Motor Speed";
const String Settings::acs_weight = "ACS: Low Pass weight";
const String Settings::acs_on_atv = "ACS: On Threshold Apple TV";
const String Settings::acs_off_atv = "ACS: Off Threshold Apple TV";
const String Settings::hs_update_interval = "Homespan: ACS Update Interval";

std::map<String, float> params = {
  {Settings::steps, 200},
  {Settings::speed, 120},
  {Settings::acs_weight, 0.01},
  {Settings::acs_on_atv, 83},
  {Settings::acs_off_atv, 76},
  {Settings::hs_update_interval, 30},
};

String hashString(String str) {
    hash<std::string> hasher;
    return String(hasher(std::string(str.c_str())));
}

int openNS() {
  esp_err_t err = nvs_open(setting_ns, NVS_READWRITE, &nvs);
  if (err != ESP_OK) {
    Serial.println("An Error has occurred while opening NVS namespace 'settings'");
  }
  return err;
}

void updateSetting(String key, float val) {
  if (params.find(key) == params.end()) {
    Serial.println("Couldn't find parameter...");
    return;
  }
  params[key] = val;
  if (openNS() != ESP_OK) {
    return;
  }
  esp_err_t err;
  if (err = nvs_set_blob(nvs, hashString(key).c_str(), &val, sizeof(float)); err != ESP_OK) {
    Serial.printf("Error occurred while writing to NVS %d\n", err);
    nvs_close(nvs);
    return;
  }
  if (nvs_commit(nvs) != ESP_OK) {
    Serial.println("Couldn't commit to NVS");
  }
  nvs_close(nvs);
}

void loadSettings() {
  if (nvs_flash_init() != ESP_OK) {
    Serial.println("Couldn't init nvs partition");
  }
  if (openNS() != ESP_OK) {
    return;
  }
  float value = 0;
  size_t size = sizeof(float);
  Serial.println("Loading settings");
  for (auto const& item: params) {
    esp_err_t err = nvs_get_blob(nvs, hashString(item.first).c_str(), &value, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
      continue;
    }
    if (err != ESP_OK) {
      Serial.println("Error occurred while reading setting from NVS");
      continue;
    }
    params[item.first] = value;
  }
  nvs_close(nvs);
}
