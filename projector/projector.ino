#include <Stepper.h>
#include "ACS712.h"
#include <map>

#define IN1 12
#define IN2 14
#define IN3 27
#define IN4 26
#define ACS_1 34


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


int c = 0;
float value  = 0;

Stepper motor(params[Settings::steps], IN1, IN2, IN3, IN4);
ACS712  ACS(34, 5.0, 4095, 185);


void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println(__FILE__);
  Serial.print("ACS712_LIB_VERSION: ");
  Serial.println(ACS712_LIB_VERSION);

  ACS.autoMidPoint();

  Serial.print("MidPoint: ");
  Serial.print(ACS.getMidPoint());
  Serial.print(". Noise mV: ");
  Serial.println(ACS.getNoisemV());
  ACS.suppressNoise(true);
  value = ACS.mA_AC();  // get good initial value
}


void loop()
{
  //  select sppropriate function
  float mA = ACS.mA_AC_sampling();
  float weight = params[Settings::weight];
  // float mA = ACS.mA_AC();
  value += weight * (mA - value);  // low pass filtering
  if (c % 10 == 0) {
    Serial.print("weight: ");
    Serial.print(weight);
    Serial.print(" value: ");
    Serial.print(value, 0);
    Serial.print(" mA: ");
    Serial.print(mA);
    Serial.println();
  
    c = 0;
  }
  c++;
  delay(100);
}