#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <Arduino.h>
#include <Interrupts.h>
#include <vector>
#include <memory>
#include <functional>
#include "./usb_descriptors.hpp"

namespace Input
{
    class Encoder
    {
    public:
        Encoder(uint8_t pin1, uint8_t pin2) : _pin1(pin1), _pin2(pin2) {}

        void init()
        {
            pinMode(_pin1, INPUT_PULLUP);
            pinMode(_pin2, INPUT_PULLUP);
            attachInterrupt(digitalPinToInterrupt(_pin1), &update, CHANGE, this);
            attachInterrupt(digitalPinToInterrupt(_pin2), &update, CHANGE, this);
        }

        int8_t getPosition() { return _pos; }

    private:
        uint8_t _pin1;
        uint8_t _pin2;
        uint8_t _lastState = 0;
        int8_t _pos = 0;

        static void update(Encoder *enc)
        {
            uint8_t p1val = digitalRead(17);
            uint8_t p2val = digitalRead(18);
            uint8_t state = enc->_lastState & 3;
            if (p1val)
                state |= 4;
            if (p2val)
                state |= 8;

            enc->_lastState = (state >> 2);
            switch (state)
            {
            case 1:
            case 7:
            case 8:
            case 14:
                enc->_pos = min(INT8_MAX, enc->_pos + 1);
                break;
            case 2:
            case 4:
            case 11:
            case 13:
                enc->_pos = max(INT8_MIN, enc->_pos - 1);
                break;
            case 3:
            case 12:
                enc->_pos = min(INT8_MAX, enc->_pos + 2);
                break;
            case 6:
            case 9:
                enc->_pos = max(INT8_MIN, enc->_pos - 2);
                break;
            }
        }
    };

    class Encoders
    {
    public:
        Encoders(uint8_t *encoderPins, uint8_t maxEncoderCount)
        {
            uint8_t rawCount = sizeof(encoderPins) / sizeof(uint8_t);
            uint8_t encoderCount = rawCount / 2;

            encoders = std::vector<std::unique_ptr<Encoder>>(encoderCount);
            for (uint8_t i = 0; i < encoderCount; i++)
            {
                encoders[i] = std::make_unique<Encoder>(encoderPins[i * 2], encoderPins[i * 2 + 1]);
            }
            encoderNum = min(encoderCount, maxEncoderCount);
        }

        void init()
        {
            for (uint8_t i = 0; i < encoderNum; i++)
            {
                encoders[i]->init();
            }
        }

        std::vector<int8_t> getEncoderValues()
        {
            std::vector<int8_t> values(encoderNum);
            for (uint8_t i = 0; i < encoderNum; i++)
            {
                values[i] = encoders[i]->getPosition();
            }
            return values;
        }

    private:
        std::vector<std::unique_ptr<Encoder>> encoders;
        uint8_t encoderNum;
    };

}

#endif // ENCODER_HPP
