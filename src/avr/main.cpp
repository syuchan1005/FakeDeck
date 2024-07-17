#include <Arduino.h>
#include "./RotaryEncoder.hpp"
#include "./SPISlave.hpp"

void setup()
{
    // set system clock prescaler start(0x80) -> /1(0x00), /8(0x03)
    CLKPR = 0x80;
    CLKPR = 0x00;

    initRotaryEncoders();

    spiX_initslave(0);
    sei(); // Enable interrupts
}

uint16_t inputData = 0;
uint16_t oldInputData = 0;

void loop()
{
    oldInputData = inputData;

    inputData = 0;
    createRotaryEncoderData(0, RE1_PR_DATA_REG, RE1_A, RE1_B, RE1_PR_DATA_REG_S, RE1_S, &inputData, &oldInputData, &sendData);
    createRotaryEncoderData(4, RE2_PR_DATA_REG, RE2_A, RE2_B, RE2_PR_DATA_REG, RE2_S, &inputData, &oldInputData, &sendData);
    createRotaryEncoderData(8, RE3_PR_DATA_REG, RE3_A, RE3_B, RE3_PR_DATA_REG, RE3_S, &inputData, &oldInputData, &sendData);
    createRotaryEncoderData(12, RE4_PR_DATA_REG, RE4_A, RE4_B, RE4_PR_DATA_REG_S, RE4_S, &inputData, &oldInputData, &sendData);

    delay(2);
}
