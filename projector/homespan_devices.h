#include "HomeSpan.h"

extern ACS712 ACS_ATV;
extern ACS712 ACS_PROJECTOR;
extern float mA_value_ATV;
void turnOn();
void turnOff();

struct Projector : Service::LightBulb {
    SpanCharacteristic *state = new Characteristic::Active(0);

    // Projector() : Service::LightBulb() {
    // }
    boolean update() {
        if (state->updated()) {
            if (state->getNewVal()) {
                turnOn();
            }
            turnOff();
        }
        return true;
    }
};

struct AppleTV:Service::LightSensor {
    SpanCharacteristic *currentValue = new Characteristic::CurrentAmbientLightLevel(0);
    int counter = 0;

    void loop() {
        if (currentValue->timeVal() > 10) {
            float weight = params[Settings::acs_weight];
            float mA = ACS_ATV.mA_AC();
            mA_value_ATV += weight * (mA - mA_value_ATV);  // low pass filtering
            if (counter % int(params[Settings::hs_update_interval]) == 0) {
                currentValue->setVal(mA_value_ATV, true);
                counter = 0;
            } else {
                currentValue->setVal(mA_value_ATV, false);
                counter++;
            }
        }
    }
};