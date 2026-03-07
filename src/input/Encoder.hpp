#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <Arduino.h>
#include "./usb_descriptors.hpp"

// Encoder GPIO pin assignments (replaces ATtiny2313 + SPI)
// These pins are freed from SPI0 (GP16-19) + remaining available GPIOs
#define ENC1_A 16
#define ENC1_B 17
#define ENC2_A 18
#define ENC2_B 19
#define ENC3_A 22
#define ENC3_B 27
#define ENC4_A 28
#define ENC4_B 0
// GP0 is safe to use as GPIO: arduino-pico maps Serial to USB CDC, not UART0.
// UART0 is Serial1, so GP0/GP1 are free for general GPIO use.

// SW inputs: all 4 push switches share a single ADC pin via a binary-weighted
// resistor ladder. Each switch has a resistor with 2^n weighting (1k, 2k, 4k, 8k).
// The ADC value uniquely identifies any combination of pressed switches (16 patterns).
#define ENC_SW_ADC 26  // ADC0 (GP26)

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

        // Accumulated rotation deltas, updated by GPIO interrupts on Core1.
        // Declared volatile because they are written in ISRs and read in get_event().
        volatile int8_t enc_delta[4] = {0, 0, 0, 0};

        // Gray-code state machine for robust quadrature decoding.
        // Tracks the last AB state per encoder to detect valid transitions only.
        // prev_ab[i] holds the previous 2-bit AB value (bit1=A, bit0=B).
        //
        // Valid transition table (standard quadrature):
        //   CW  sequence: 00 → 10 → 11 → 01 → 00  (A leads B)
        //   CCW sequence: 00 → 01 → 11 → 10 → 00  (B leads A)
        //
        // transition_table[prev][curr]:
        //   +1 = CW step, -1 = CCW step, 0 = no change / invalid (bounce)
        static const int8_t transition_table[4][4] = {
            // curr: 00   01   10   11
            {  0,  -1,  +1,   0 }, // prev=00
            { +1,   0,   0,  -1 }, // prev=01
            { -1,   0,   0,  +1 }, // prev=10
            {  0,  +1,  -1,   0 }, // prev=11
        };

        volatile uint8_t prev_ab[4] = {0, 0, 0, 0};

        void isr_enc1()
        {
            uint8_t ab = (digitalRead(ENC1_A) << 1) | digitalRead(ENC1_B);
            enc_delta[0] += transition_table[prev_ab[0]][ab];
            prev_ab[0] = ab;
        }

        void isr_enc2()
        {
            uint8_t ab = (digitalRead(ENC2_A) << 1) | digitalRead(ENC2_B);
            enc_delta[1] += transition_table[prev_ab[1]][ab];
            prev_ab[1] = ab;
        }

        void isr_enc3()
        {
            uint8_t ab = (digitalRead(ENC3_A) << 1) | digitalRead(ENC3_B);
            enc_delta[2] += transition_table[prev_ab[2]][ab];
            prev_ab[2] = ab;
        }

        void isr_enc4()
        {
            uint8_t ab = (digitalRead(ENC4_A) << 1) | digitalRead(ENC4_B);
            enc_delta[3] += transition_table[prev_ab[3]][ab];
            prev_ab[3] = ab;
        }

        // Lookup table for the binary-weighted resistor ladder SW detection.
        // Each SW connects 3.3V through its weight resistor to the ADC node.
        // A pull-down resistor (1kΩ) connects the ADC node to GND.
        // SW weights: SW1=1kΩ, SW2=2kΩ, SW3=4kΩ, SW4=8kΩ, pull-down=1kΩ.
        //
        // Voltage when switches pressed: V = 3.3 * R_pull / (R_parallel + R_pull)
        // where R_parallel is the parallel combination of pressed switch resistors.
        //
        // ADC values (12-bit, 0-4095) computed from the resistor network.
        // These are theoretical values; fine-tune from real measurements if needed.
        struct SwPattern
        {
            uint16_t adc_val;
            uint8_t mask; // bit0=SW1, bit1=SW2, bit2=SW3, bit3=SW4
        };

        // ADC values computed for: SW1=120Ω, SW2=220Ω, SW3=470Ω, SW4=1kΩ, R_pull=47Ω
        // Formula: V = 3.3 * R_pull / (R_parallel + R_pull), ADC = V/3.3 * 4095
        // Min separation between adjacent patterns: 64 counts → safe margin ±32
        // Sorted ascending by ADC value for binary-search friendliness.
        const SwPattern SW_PATTERNS[16] = {
            {   0, 0b0000}, // none
            { 184, 0b1000}, // SW4
            { 372, 0b0100}, // SW3
            { 525, 0b1100}, // SW3+SW4
            { 721, 0b0010}, // SW2
            { 847, 0b1010}, // SW2+SW4
            { 978, 0b0110}, // SW2+SW3
            {1085, 0b1110}, // SW2+SW3+SW4
            {1152, 0b0001}, // SW1
            {1249, 0b1001}, // SW1+SW4
            {1350, 0b0101}, // SW1+SW3
            {1434, 0b1101}, // SW1+SW3+SW4
            {1544, 0b0011}, // SW1+SW2
            {1617, 0b1011}, // SW1+SW2+SW4
            {1694, 0b0111}, // SW1+SW2+SW3
            {1758, 0b1111}, // all
        };

        uint8_t read_sw_state()
        {
            uint16_t adc = analogRead(ENC_SW_ADC);
            uint8_t best_mask = 0;
            uint16_t best_diff = 0xFFFF;
            for (const auto &p : SW_PATTERNS)
            {
                uint16_t diff = (uint16_t)abs((int)adc - (int)p.adc_val);
                if (diff < best_diff)
                {
                    best_diff = diff;
                    best_mask = p.mask;
                }
            }
            return best_mask;
        }

        class Encoder
        {
        public:
            void init()
            {
#if defined(DECK_TOUCH)
                // Configure encoder A/B pins as inputs with pull-ups
                pinMode(ENC1_A, INPUT_PULLUP);
                pinMode(ENC1_B, INPUT_PULLUP);
                pinMode(ENC2_A, INPUT_PULLUP);
                pinMode(ENC2_B, INPUT_PULLUP);
                pinMode(ENC3_A, INPUT_PULLUP);
                pinMode(ENC3_B, INPUT_PULLUP);
                pinMode(ENC4_A, INPUT_PULLUP);
                pinMode(ENC4_B, INPUT_PULLUP);

                // Attach interrupts on BOTH A and B pins (CHANGE) for full gray-code tracking.
                // The state machine in the ISR ignores invalid transitions caused by bounce.
                // Calling from setup1() means ISRs run on Core1
                attachInterrupt(digitalPinToInterrupt(ENC1_A), isr_enc1, CHANGE);
                attachInterrupt(digitalPinToInterrupt(ENC1_B), isr_enc1, CHANGE);
                attachInterrupt(digitalPinToInterrupt(ENC2_A), isr_enc2, CHANGE);
                attachInterrupt(digitalPinToInterrupt(ENC2_B), isr_enc2, CHANGE);
                attachInterrupt(digitalPinToInterrupt(ENC3_A), isr_enc3, CHANGE);
                attachInterrupt(digitalPinToInterrupt(ENC3_B), isr_enc3, CHANGE);
                attachInterrupt(digitalPinToInterrupt(ENC4_A), isr_enc4, CHANGE);
                attachInterrupt(digitalPinToInterrupt(ENC4_B), isr_enc4, CHANGE);

                // Read actual pin state now so the first ISR call has a correct baseline.
                // Without this, prev_ab starts at 0 but the encoder rests at AB=3 (detent),
                // causing the first transition to be misread.
                prev_ab[0] = (digitalRead(ENC1_A) << 1) | digitalRead(ENC1_B);
                prev_ab[1] = (digitalRead(ENC2_A) << 1) | digitalRead(ENC2_B);
                prev_ab[2] = (digitalRead(ENC3_A) << 1) | digitalRead(ENC3_B);
                prev_ab[3] = (digitalRead(ENC4_A) << 1) | digitalRead(ENC4_B);

                // Configure SW ADC pin
                pinMode(ENC_SW_ADC, INPUT);
                analogReadResolution(12); // 12-bit ADC (0-4095)
#endif
            }

            Event::EventDataHolder get_event()
            {
#if defined(DECK_TOUCH)
                // Atomically consume rotation deltas.
                // This encoder has detents at AB=11. Each click produces 4 valid
                // gray-code transitions (3->1->0->2->3 CW, or 3->2->0->1->3 CCW),
                // so threshold is ±4: one full detent = exactly 4 counts.
                // If the sign flips mid-detent (bounce reversal), reset to avoid
                // a stale partial count triggering on the next real click.
                uint8_t turn = 0;
                noInterrupts();
                for (int i = 0; i < 4; i++)
                {
                    int8_t d = enc_delta[i];
                    if (d >= 4)
                    {
                        enc_delta[i] = 0;
                        turn |= (2 << (i * 2)); // CW  (bit1 of pair)
                    }
                    else if (d <= -4)
                    {
                        enc_delta[i] = 0;
                        turn |= (1 << (i * 2)); // CCW (bit0 of pair)
                    }
                    else if (d > 0 && enc_delta[i] < 0)
                    {
                        enc_delta[i] = 0; // direction reversed mid-detent, reset
                    }
                    else if (d < 0 && enc_delta[i] > 0)
                    {
                        enc_delta[i] = 0; // direction reversed mid-detent, reset
                    }
                }
                interrupts();

                uint8_t press = read_sw_state();
                previousEvent = Event::EventDataHolder(turn, press);
#endif
                return previousEvent;
            }

        private:
            Event::EventDataHolder previousEvent = Event::EventDataHolder(0, 0);
        };
    };
};

#endif // ENCODER_HPP
