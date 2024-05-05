#ifndef LCD_HPP
#define LCD_HPP

#include <memory>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <XPT2046_Touchscreen.h>
#include "../usb_descriptors.hpp"
#include "../FileRepository.hpp"

#define CALIBRATION_FILE "/lcd_calibration.dat"

namespace Input
{
    TFT_eSPI tft;

    class Touch
    {
    public:
        virtual void setTouch(uint16_t *parameters) = 0;
        virtual void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size) = 0;
        virtual uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t threshold) = 0;
    };

#ifndef RP2040_PIO_SPI
    class TFT_Touch : public Touch
    {
    public:
        void setTouch(uint16_t *parameters)
        {
            tft.setTouch(parameters);
        }

        void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size)
        {
            tft.calibrateTouch(parameters, color_fg, color_bg, size);
        }

        uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t threshold)
        {
            return tft.getTouch(x, y, threshold);
        }
    };

    std::unique_ptr<Touch> touch = std::make_unique<TFT_Touch>();

#else

    class XPT2046_Touch : public Touch
    {
    public:
        XPT2046_Touch()
        {
            SPI1.setRX(TFT_MISO2);
            SPI1.setTX(TFT_MOSI2);
            SPI1.setSCK(TFT_SCLK2);
            touchscreen.begin(SPI1);
        }

        void setTouch(uint16_t *parameters)
        {
            // touch.setCalibration(parameters);
        }

        void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size)
        {
            // touch.calibrateTouch(parameters, color_fg, color_bg, size);
        }

        uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t threshold)
        {
            TS_Point p = touchscreen.getPoint();
            if (p.z < threshold)
                return 0;

            *x = map(p.x, 3940, 220, 0, tft.width());
            *y = map(p.y, 3870, 310, 0, tft.height());
            return 1;
        }

    private:
        XPT2046_Touchscreen touchscreen = XPT2046_Touchscreen(TOUCH_CS2);
    };

    std::unique_ptr<Touch> touch = std::make_unique<XPT2046_Touch>();

#endif // RP2040_PIO_SPI
    class LCD
    {
    public:
        void init()
        {
            TJpgDec.setSwapBytes(true);
            TJpgDec.setCallback(LCD::tft_output);

            tft.init();
            tft.setRotation(1);
            tft.fillScreen(TFT_BLACK);

            pinMode(TFT_LED, OUTPUT);
            analogWrite(TFT_LED, 0xFF);
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
                touch->setTouch(calData);
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

                touch->calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

                tft.fillScreen(TFT_BLACK);

                file_repository.writeFile(CALIBRATION_FILE, (uint8_t *)calData, 14);
                Serial.printf("Calibration data saved: %d, %d, %d, %d, %d\n", calData[0], calData[1], calData[2], calData[3], calData[4]);
            }
        }

        void draw_key_image(uint8_t key_index, uint8_t *buffer, uint16_t buffer_size)
        {
            uint8_t lcd_key_index = KEY_COUNT - key_index - 1;
            int16_t x_spacing = (tft.width() - KEY_IMAGE_SIZE * KEY_COUNT_COL) / (KEY_COUNT_COL + 1);
            int16_t x = x_spacing + (KEY_IMAGE_SIZE + x_spacing) * (lcd_key_index % KEY_COUNT_COL);
            int16_t y_spacing = (tft.height() - KEY_IMAGE_SIZE * KEY_COUNT_ROW) / (KEY_COUNT_ROW + 1);
            int16_t y = y_spacing + (KEY_IMAGE_SIZE + y_spacing) * (lcd_key_index / KEY_COUNT_COL);
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
            uint8_t isTouched = touch->getTouch(&x, &y, 600);
            if (isTouched)
            {
#ifdef DEBUG_TOUCH
                tft.setCursor(5, 5, 2);
                tft.printf("x: %i     ", x);
                tft.setCursor(5, 20, 2);
                tft.printf("y: %i    ", y);
                tft.drawPixel(x, y, TFT_RED);
#endif

                int16_t x_spacing = (tft.width() - KEY_IMAGE_SIZE * KEY_COUNT_COL) / (KEY_COUNT_COL + 1);
                int16_t y_spacing = (tft.height() - KEY_IMAGE_SIZE * KEY_COUNT_ROW) / (KEY_COUNT_ROW + 1);
                for (uint8_t i = 0; i < KEY_COUNT; i++)
                {
                    int16_t x_start = x_spacing + (KEY_IMAGE_SIZE + x_spacing) * (i % KEY_COUNT_COL);
                    int16_t x_end = x_start + KEY_IMAGE_SIZE;
                    int16_t y_start = y_spacing + (KEY_IMAGE_SIZE + y_spacing) * (i / KEY_COUNT_COL);
                    int16_t y_end = y_start + KEY_IMAGE_SIZE;
                    if (x >= x_start && x < x_end && y >= y_start && y < y_end)
                    {
                        return KEY_COUNT - i - 1;
                    }
                }
            }
            return 0x80;
        }

        /**
         * @brief Set the brightness
         * 
         * @param brightness 0-100
         */
        void set_brightness(uint8_t brightness)
        {
            analogWrite(TFT_LED, map(brightness, 0, 100, 0, 0xFF));
        }

    private:
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
}

#endif // LCD_HPP
