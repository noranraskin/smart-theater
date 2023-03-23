#include "HomeSpan.h"

extern ACS712 ACS_PROJECTOR;
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
