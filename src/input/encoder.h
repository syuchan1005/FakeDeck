#include <Arduino.h>
#include <Interrupts.h>

class Encoder
{
public:
    Encoder(uint8_t pin1, uint8_t pin2)
    {
        _pin1 = pin1;
        _pin2 = pin2;
    };

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
        {
            state |= 4;
        }
        if (p2val)
        {
            state |= 8;
        }

        enc->_lastState = (state >> 2);
        switch (state)
        {
        case 1:
        case 7:
        case 8:
        case 14:
            enc->_pos = min(INT8_MAX, enc->_pos + 1);
            return;
        case 2:
        case 4:
        case 11:
        case 13:
            enc->_pos = max(INT8_MIN, enc->_pos - 1);
            return;
        case 3:
        case 12:
            enc->_pos = min(INT8_MAX, enc->_pos + 2);
            return;
        case 6:
        case 9:
            enc->_pos = max(INT8_MIN, enc->_pos - 2);
            return;
        }
    }
};
