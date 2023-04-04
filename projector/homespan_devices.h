#include "HomeSpan.h"
#include "ACS712.h"

extern ACS712 ACS_PROJECTOR;
void turnOn();
void turnOff();

struct Projector : Service::LightBulb {
    SpanCharacteristic *state = new Characteristic::On(0);

    Projector() : Service::LightBulb() {

    }
    // Projector() : Service::LightBulb() {
    // }
    boolean update() {
        if (state->getNewVal()) {
            turnOn();
        }
        turnOff();
        return true;
    }
    void loop() {
        float projector_val = ACS_PROJECTOR.mA_AC();
        if (projector_val > params[Settings::projector_on_thresh]) {
            if (!state->getVal()) {
                state->setVal(1);
            }
        }
        if (projector_val < params[Settings::projector_off_thresh]) {
            if (state->getVal()) {
                state->setVal(0);
            }
        }
    }
};
