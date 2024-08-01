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
    namespace Encoder
    {
        namespace Event
        {
            class EventDataHolder
            {
            public:
                EventDataHolder(uint8_t turn, uint8_t press) : turn(turn), press(press) {}
                uint8_t turn;
                uint8_t press;
            };

            enum EventType
            {
                NONE,
                TURN,
                PRESS
            };

            class Event
            {
            public:
                Event(EventType type, uint8_t value) : type(type), value(value) {}
                EventType type;
                uint8_t value;
            };
        };

        class Encoder
        {
        public:
            void init()
            {

#if defined(DECK_TOUCH)
                SPI.begin();
                SPI.beginTransaction(SPISettings(8000000UL / 8 / 16, MSBFIRST, SPI_MODE0));
                pinMode(17, INPUT);
#endif
            }

            Event::EventDataHolder get_event()
            {
#if defined(DECK_TOUCH)
                SPI.transfer(0);
                uint8_t lowerRxbuf = SPI.transfer(0);
                SPI.transfer(1);
                uint8_t upperRxbuf = SPI.transfer(1);
                uint16_t rxbuf = (upperRxbuf << 8) | lowerRxbuf;
                if (rxbuf >= 0x8000)
                    previousEvent = Event::EventDataHolder(lowerRxbuf, upperRxbuf & 0x7F);
#endif
                return previousEvent;
            }

        private:
            Event::EventDataHolder previousEvent = Event::EventDataHolder(0, 0);
        };
    };
};

#endif // ENCODER_HPP
