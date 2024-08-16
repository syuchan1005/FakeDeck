#ifndef LCD_HPP
#define LCD_HPP

#include <memory>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "../usb_descriptors.hpp"
#include "../FileRepository.hpp"

#if defined(USE_TJPG)
#include <TJpg_Decoder.h>
#else
#include <JPEGDEC.h>
#endif

#define CALIBRATION_FILE "/lcd_calibration.dat"

namespace Input
{
    namespace Display
    {
        TFT_eSPI tft;

#if !defined(USE_TJPG)
        JPEGDEC jpeg;
#endif

        class Touch
        {
        public:
            virtual void setTouch(uint16_t *parameters) = 0;
            virtual void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size) = 0;
            virtual uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t threshold) = 0;
        };

#if !defined(USE_ORIGINAL_TOUCH)
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

                *x = map(TOUCH_X, 0, tft.width());
                *y = map(TOUCH_Y, 0, tft.height());
                return 1;
            }

        private:
            XPT2046_Touchscreen touchscreen = XPT2046_Touchscreen(TOUCH_CS2);
        };

        std::unique_ptr<Touch> touch = std::make_unique<XPT2046_Touch>();

#endif

        const uint8_t NO_KEY_PRESSED = 0x80;
        const uint16_t TOUCH_TAP_THRESHOLD_PX = 100;
        const uint32_t TOUCH_LONG_TAP_THRESHOLD_MS = 500;

        namespace Event
        {
            enum EventType
            {
                NONE,
                KEY_PRESSED,
                TOUCH_PRESSED_SHORT,
                TOUCH_PRESSED_LONG,
                TOUCH_DRAG
            };

            class Event
            {
            public:
                Event(
                    EventType type,
                    int8_t keyIndex = -1,
                    int16_t x = -1,
                    int16_t y = -1,
                    int16_t x_out = -1,
                    int16_t y_out = -1) : type(type), keyIndex(keyIndex), x(x), y(y), x_out(x_out), y_out(y_out) {}

                EventType type;
                uint8_t keyIndex;
                uint16_t x;
                uint16_t y;
                uint16_t x_out;
                uint16_t y_out;
            };

            Event NONE_OBJ = Event(EventType::NONE);
        };

        class LCD
        {
        public:
            void init()
            {
#if defined(USE_TJPG)
                TJpgDec.setSwapBytes(true);
                TJpgDec.setCallback(LCD::draw_image_callback);
#endif
                tft.init();
                tft.setRotation(TFT_ROTATION);
                tft.fillScreen(TFT_BLACK);

                pinMode(TFT_LED, OUTPUT);
                analogWrite(TFT_LED, 0xFF);
            }

            void calibrate(FileRepository &file_repository, bool force_calibration = false)
            {
                uint16_t calData[5];
                bool hasCalData = false;
                if (!force_calibration &&
                    file_repository.readFile(CALIBRATION_FILE, (uint8_t *)calData, 14) == 14)
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
#if defined(KEY_ORDER_REVERSE)
                uint8_t lcd_key_index = KEY_COUNT - key_index - 1;
#else
                uint8_t lcd_key_index = key_index;
#endif
                int16_t x = KEY_X(tft.width(), lcd_key_index);
                int16_t y = KEY_Y(tft.height(), lcd_key_index);

#if defined(USE_TJPG)
                TJpgDec.drawJpg(x, y, buffer, buffer_size);
#else
                if (jpeg.openRAM(buffer, buffer_size, LCD::JPEGDraw))
                {
                    jpeg.decode(x, y, 0);
                    jpeg.close();
                }
#endif

                tft.drawRoundRect(x - 2, y - 2, KEY_IMAGE_SIZE + 4, KEY_IMAGE_SIZE + 4, 8, TFT_WHITE);
            }

            void draw_touch_image(uint16_t rawX, uint16_t rawY, uint8_t *buffer, uint16_t buffer_size)
            {
#if defined(DECK_TOUCH)
                uint16_t y = rawY + TOUCH_OFFSET_Y(tft.height());
#if defined(USE_TJPG)
                TJpgDec.drawJpg(rawX, y, buffer, buffer_size);
#else
                if (jpeg.openRAM(buffer, buffer_size, LCD::JPEGDraw))
                {
                    jpeg.decode(rawX, y, 0);
                    jpeg.close();
                }
#endif // USE_TJPG
#endif // DECK_TOUCH
            }

            /**
             * @brief Get the pressed button index
             *
             * @return uint8_t if no button is pressed, returns [NO_KEY_PRESSED].
             */ 
            uint8_t get_pressed_button()
            {
                uint16_t x, y;
                uint8_t isTouched = touch->getTouch(&x, &y, TOUCH_THRESHOLD);
                if (isTouched)
                {
#ifdef DEBUG_TOUCH
                    tft.setCursor(5, 5, 2);
                    tft.printf("x: %i     ", x);
                    tft.setCursor(5, 20, 2);
                    tft.printf("y: %i    ", y);
                    tft.drawPixel(x, y, TFT_RED);
#endif

                    for (uint8_t i = 0; i < KEY_COUNT; i++)
                    {
                        int16_t x_start = KEY_X(tft.width(), i);
                        int16_t y_start = KEY_Y(tft.height(), i);
                        int16_t x_end = x_start + KEY_IMAGE_SIZE;
                        int16_t y_end = y_start + KEY_IMAGE_SIZE;
                        if (x >= x_start && x < x_end && y >= y_start && y < y_end)
                        {
#if defined(KEY_ORDER_REVERSE)
                            return KEY_COUNT - i - 1;
#else
                            return i;
#endif
                        }
                    }
                }
                return NO_KEY_PRESSED;
            }

            int16_t touch_start_x = -1;
            int16_t touch_start_y = -1;
            uint32_t touch_start_ms = 0;
            int16_t previous_x = -1;
            int16_t previous_y = -1;
            /**
             * @brief Gets the event from the LCD.
             *
             * @return Event::Event
             */
            Event::Event get_event()
            {
#if defined(DECK_TOUCH)
                uint16_t x, y;
                uint8_t is_touched = touch->getTouch(&x, &y, 600);
                if (!is_touched)
                {
                    Event::Event event = Event::NONE_OBJ;
                    if (touch_start_x != -1)
                    {
                        uint16_t distance = sqrt(pow(touch_start_x - previous_x, 2) + pow(touch_start_y - previous_y, 2));
                        uint32_t touch_duration = millis() - touch_start_ms;
                        if (distance > TOUCH_TAP_THRESHOLD_PX)
                        {
                            if (previous_x != -1 && previous_y != -1)
                            {
                                event = Event::Event(Event::EventType::TOUCH_DRAG, -1, touch_start_x, TOUCH_NORMALIZED_Y(tft.height(), touch_start_y), previous_x, TOUCH_NORMALIZED_Y(tft.height(), previous_y));
                            }
                        }
                        else if (touch_duration < TOUCH_LONG_TAP_THRESHOLD_MS)
                        {
                            event = Event::Event(Event::EventType::TOUCH_PRESSED_SHORT, -1, touch_start_x, TOUCH_NORMALIZED_Y(tft.height(), touch_start_y));
                        }
                        else
                        {
                            event = Event::Event(Event::EventType::TOUCH_PRESSED_LONG, -1, touch_start_x, TOUCH_NORMALIZED_Y(tft.height(), touch_start_y));
                        }
                    }

                    touch_start_x = -1;
                    touch_start_y = -1;
                    previous_x = -1;
                    previous_y = -1;
                    return event;
                }

                previous_x = x;
                previous_y = y;

                if (y > TOUCH_OFFSET_Y(tft.height()))
                {
                    if (touch_start_x == -1)
                    {
                        touch_start_x = x;
                        touch_start_y = y;
                        touch_start_ms = millis();
                    }
                    return Event::NONE_OBJ;
                }

#endif // DECK_TOUCH
                uint8_t pressedButton = get_pressed_button();
                if (pressedButton != NO_KEY_PRESSED)
                {
                    return Event::Event(Event::EventType::KEY_PRESSED, pressedButton);
                }
                return Event::NONE_OBJ;
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
#if defined(USE_TJPG)
            static bool draw_image_callback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
            {
                // Stop further decoding as image is running off bottom of screen
                if (y >= tft.height())
                    return 0;

                // This function will clip the image block rendering automatically at the TFT boundaries
                tft.pushImage(x, y, w, h, bitmap);

                // Return 1 to decode next block
                return 1;
            }
#else
            static int JPEGDraw(JPEGDRAW *pDraw)
            {
                // swap bytes
                for (uint32_t i = 0; i < pDraw->iWidth * pDraw->iHeight; i++)
                {
                    uint16_t color = pDraw->pPixels[i];
                    pDraw->pPixels[i] = (color << 8) | (color >> 8);
                }

                tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
                return 1;
            }
#endif
        };
    };
}

#endif // LCD_HPP
