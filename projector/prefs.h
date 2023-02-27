#include <map>

struct Settings {
  static const String steps;
  static const String speed;
  static const String weight;
};

const String Settings::steps = "Stepper: Steps per Revolution";
const String Settings::speed = "Stepper: Motor Speed";
const String Settings::weight = "ACS: Low Pass weight";

std::map<String, float> params = {
  {Settings::steps, 200},
  {Settings::speed, 120},
  {Settings::weight, 0.01}
};
