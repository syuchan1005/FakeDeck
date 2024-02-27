#include <Arduino.h>
#include "./usb_descriptors.h"

class Buttons {
    public:
        Buttons(uint8_t *buttonsPins) {
            pins = buttonsPins;
            pinsNum = min(sizeof(buttonsPins) / sizeof(uint8_t), KEY_COUNT);
        };

        void init() {
            for (uint8_t i = 0; i < pinsNum; i++) {
                pinMode(pins[i], INPUT_PULLUP);
            }
        }

        void getButtons(uint8_t *buttonValues, uint8_t offset) {
            for (uint8_t i = 0; i < pinsNum; i++) {
                buttonValues[i + offset] = !digitalRead(pins[i]);
            }
        };

    private:
        uint8_t *pins;
        uint8_t pinsNum;
};
