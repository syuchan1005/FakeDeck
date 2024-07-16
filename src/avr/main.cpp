#include <Arduino.h>
#include "./SPISlave.hpp"

void setup()
{
    // set system clock prescaler start(0x80) -> /1(0x00), /8(0x03)
    CLKPR = 0x80;
    CLKPR = 0x00;

    // DDRD |= (1 << PD6);   // Set PD6 as output
    // PORTD |= (1 << PD6); // Set PD6 HIGH
    // PORTD &= ~(1 << PD6); // Set PD6 LOW

    spiX_initslave(0);
    sei(); // Enable interrupts
}

void loop()
{
    if (spiX_put(0b01010101))
    {
        spiX_wait();
        if (spiX_put(0b10101010))
            spiX_wait();
    }
    delay(1000);
}
