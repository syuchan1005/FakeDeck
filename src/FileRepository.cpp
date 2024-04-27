#include <Arduino.h>
#include <LittleFS.h>

#ifndef FILE_REPOSITORY_H
#define FILE_REPOSITORY_H

class FileRepository
{
public:
    void init()
    {
        if (!LittleFS.begin())
        {
            LittleFS.format();
            LittleFS.begin();
        }
    }

    bool readFile(const char *filename, uint8_t *buffer, uint16_t buffer_size)
    {
        File file = LittleFS.open(filename, "r");
        if (!file) return false;
        int read_size = file.read(buffer, buffer_size);
        file.close();
        return read_size == buffer_size;
    }

    void writeFile(const char *filename, uint8_t *buffer, uint16_t buffer_size)
    {
        File file = LittleFS.open(filename, "w");
        if (!file) return;
        file.write(buffer, buffer_size);
        file.close();
    }
};

#endif
