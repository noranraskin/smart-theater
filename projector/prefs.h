#include <map>
#include <nvs.h>
#include <nvs_flash.h>

using std::hash;

nvs_handle_t nvs;
const char* setting_ns = "settings";

struct Settings {
  static const String acceleration;
  static const String steps;
  static const String speed;
  static const String thresh_on_atv;
  static const String thresh_off_atv;
  static const String projector_on_thresh;
  static const String projector_off_thresh;
  static const String spanpoint_en;
};

const String Settings::acceleration = "Stepper acceleration";
const String Settings::steps = "Stepper steps to open or close";
const String Settings::speed = "Stepper motor speed";
const String Settings::thresh_on_atv = "On threshold for Apple TV";
const String Settings::thresh_off_atv = "Off threshold for Apple TV";
const String Settings::projector_on_thresh = "On threshold for Projector";
const String Settings::projector_off_thresh = "Off threshold for Projector";
const String Settings::spanpoint_en = "SpanPoint enable";

std::map<String, int> params = {
  {Settings::acceleration, 8000},
  {Settings::steps, 4400},
  {Settings::speed, 600},
  {Settings::thresh_on_atv, 2000},
  {Settings::thresh_off_atv, 100},
  {Settings::projector_on_thresh, 2000},
  {Settings::projector_on_thresh, 1000},
  {Settings::spanpoint_en, 1},
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

void updateSetting(String key, int val) {
  if (params.find(key) == params.end()) {
    Serial.println("Couldn't find parameter...");
    return;
  }
  params[key] = val;
  if (openNS() != ESP_OK) {
    return;
  }
  esp_err_t err;
  if (err = nvs_set_blob(nvs, hashString(key).c_str(), &val, sizeof(int)); err != ESP_OK) {
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
  int value = 0;
  size_t size = sizeof(int);
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
