#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

#include "../usb_descriptors.h"
#include "../FileRepository.cpp"

#define CALIBRATION_FILE "/lcd_calibration.dat"

class LCD
{
public:
    void init()
    {
        TJpgDec.setSwapBytes(true);
        TJpgDec.setCallback(this->tft_output);

        tft.init();
        tft.setRotation(1);
        tft.fillScreen(TFT_BLACK);
    }

    void calibrate(FileRepository &file_repository, bool force_calibration = false)
    {
        uint16_t calData[5];
        bool hasCalData = false;
        if (!force_calibration &&
            file_repository.readFile(CALIBRATION_FILE, (uint8_t *)calData, 14))
        {
            hasCalData = true;
        }

        if (hasCalData)
        {
            Serial.printf("Calibration data loaded: %d, %d, %d, %d, %d\n", calData[0], calData[1], calData[2], calData[3], calData[4]);
            tft.setTouch(calData);
        }
        else
        {
            tft.setCursor(20, 0);
            tft.setTextFont(2);
            tft.setTextSize(1);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);

            tft.println("Touch corners as indicated");

            tft.setTextFont(1);
            tft.println();

            tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

            tft.fillScreen(TFT_BLACK);

            file_repository.writeFile(CALIBRATION_FILE, (uint8_t *)calData, 14);
            Serial.printf("Calibration data saved: %d, %d, %d, %d, %d\n", calData[0], calData[1], calData[2], calData[3], calData[4]);
        }
    }

    void draw_key_image(FileRepository &file_repository, uint8_t key_index, uint8_t *buffer, uint16_t buffer_size)
    {
        uint8_t lcd_key_index = KEY_COUNT - key_index - 1;
        int16_t x_spacing = (tft.width() - KEY_PIXEL_WIDTH * KEY_COUNT_COL) / (KEY_COUNT_COL + 1);
        int16_t x = x_spacing + (KEY_PIXEL_WIDTH + x_spacing) * (lcd_key_index % KEY_COUNT_COL);
        int16_t y_spacing = (tft.height() - KEY_PIXEL_HEIGHT * KEY_COUNT_ROW) / (KEY_COUNT_ROW + 1);
        int16_t y = y_spacing + (KEY_PIXEL_HEIGHT + y_spacing) * (lcd_key_index / KEY_COUNT_COL);
        TJpgDec.drawJpg(x, y, buffer, buffer_size);
    }

    /**
     * @brief Get the pressed button index
     *
     * @return uint8_t if no button is pressed, returns 0x80 (first bit is set to 1)
     */
    uint8_t get_pressed_button()
    {
        uint16_t x, y;
        uint8_t isTouched = tft.getTouch(&x, &y);
        //Serial.printf("isTouched: %d, x: %d, y: %d\n", isTouched, x, y);
        if (isTouched)
        {
            tft.setCursor(5, 5, 2);
            tft.printf("x: %i     ", x);
            tft.setCursor(5, 20, 2);
            tft.printf("y: %i    ", y);

            tft.drawPixel(x, y, TFT_RED);

            int16_t x_spacing = (tft.width() - KEY_PIXEL_WIDTH * KEY_COUNT_COL) / (KEY_COUNT_COL + 1);
            int16_t y_spacing = (tft.height() - KEY_PIXEL_HEIGHT * KEY_COUNT_ROW) / (KEY_COUNT_ROW + 1);
            for (uint8_t i = 0; i < KEY_COUNT; i++)
            {
                int16_t x_start = x_spacing + (KEY_PIXEL_WIDTH + x_spacing) * (i % KEY_COUNT_COL);
                int16_t x_end = x_start + KEY_PIXEL_WIDTH;
                int16_t y_start = y_spacing + (KEY_PIXEL_HEIGHT + y_spacing) * (i / KEY_COUNT_COL);
                int16_t y_end = y_start + KEY_PIXEL_HEIGHT;
                if (x >= x_start && x < x_end && y >= y_start && y < y_end)
                {
                    return KEY_COUNT - i - 1;
                }
            }
        }
        return 0x80;
    }

private:
    static TFT_eSPI tft;

    static bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
    {
        // Stop further decoding as image is running off bottom of screen
        if (y >= tft.height())
            return 0;

        // This function will clip the image block rendering automatically at the TFT boundaries
        tft.pushImage(x, y, w, h, bitmap);

        // Return 1 to decode next block
        return 1;
    }
};