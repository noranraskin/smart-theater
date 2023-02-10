#include "HomeSpan.h"

struct Stereo : Service::LightBulb {
    int powerPin;
    SpanCharacteristic *power;
    
    Stereo(int pin) : Service::LightBulb() {
        power = new Characteristic::On(0, true); // Powered off by default, but save state changes to nvm
        powerPin = pin;
        digitalWrite(powerPin, !power->getVal());
    }

    boolean update() {
        digitalWrite(powerPin, !power->getNewVal());
        return true;
    }

    void loop() {
        if (power->getVal() == digitalRead(powerPin)) {
            power->setVal(!digitalRead(powerPin));
        }
    }
};

struct Canvas : Service::Television {
    SpanCharacteristic *state = new Characteristic::Active(0);
    void (*move_motor)(int);
    boolean (*isDown)();
    boolean (*isUp)();
    Canvas(void (*move)(int), boolean (*testDown)(), boolean(*testUp)()) : Service::Television() {
        move_motor = move;
        isDown = testDown;
        isUp = testUp;
    }
    boolean update() {
        if (state->updated()) {
            move_motor(state->getNewVal());
        }
        return true;
    }
    void loop() {
        if (isUp()) {
            if (state->getVal()) {
                state->setVal(0);
            }
        }
        if (isDown()) {
            if (!state->getVal()) {
                state->setVal(1);
            }
        }
    }
};