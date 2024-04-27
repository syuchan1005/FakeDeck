#include <Arduino.h>
#include <LittleFS.h>

class FileRepository
{
public:
    void writeKeyImage(uint8_t key_index, uint8_t *buffer, uint16_t buffer_size)
    {
        (void)buffer;
        // TODO
        Serial.printf("Key Image: %d, %d\n", key_index, buffer_size);
    }

    void writeTouchscreenImage(uint16_t x_pos, uint16_t y_pos, uint16_t width, uint16_t height, uint8_t *buffer, uint16_t buffer_size)
    {
        (void)buffer;
        // TODO
        Serial.printf("Touchscreen Image: %d, %d, %d, %d, %d\n", x_pos, y_pos, width, height, buffer_size);
    }
};
