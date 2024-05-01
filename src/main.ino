#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "./usb_descriptors.hpp"
#include "./input/LCD.hpp"
#include "./FileRepository.hpp"

Input::LCD lcd;
FileRepository file_repository;
Adafruit_USBD_HID usb_hid;
uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC()};

void setup()
{
    TinyUSBDevice.setManufacturerDescriptor(DECK_USB_MANUFACTURER);
    TinyUSBDevice.setProductDescriptor(DECK_USB_PRODUCT);
    TinyUSBDevice.setID(DECK_USB_VID, DECK_USB_PID);

    usb_hid.enableOutEndpoint(true);
    usb_hid.setPollInterval(2);
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid.setReportCallback(get_report_callback, pre_set_report_callback);
    usb_hid.begin();

    Serial.begin(115200);

    lcd.init();
    file_repository.init();
    lcd.calibrate(file_repository);

    Serial.println("Setup");
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);

    delay(1000);
}

uint8_t report[INPUT_REPORT_LEN] = { 0, KEY_COUNT, 0 };
void loop()
{
    delay(100);
    uint8_t pressedKey = lcd.get_pressed_button();
    if (pressedKey != 0x80)
    {
        Serial.printf("Pressed Key: %d\n", pressedKey);
        report[pressedKey + 3] = 1;
    }
    usb_hid.sendReport(1, report, sizeof(report));
    memset(report + 3, 0, KEY_COUNT);

    // maybeSendDialReport();

    /*
    uint8_t touchscreenReport[INPUT_REPORT_LEN] = {
        0x02, 0x01, 0x00,    // touch, count(?), ?
        0x01,                // short
        U16_TO_U8S_LE(0x0A), // x
        U16_TO_U8S_LE(0x0A)  // y
    };
    See more: py\Lib\site-packages\StreamDeck\Devices\StreamDeckPlus.py#L356
    */
}

/*
std::vector<int8_t> dialValues;
void maybeSendDialReport() {
    bool isDialChanged = false;
    std::vector<int8_t> newDialValues = encoders.getEncoderValues();
    uint8_t dialReport[INPUT_REPORT_LEN] = {
        0x03, DIAL_COUNT, 0x00, // dial, count, ?
        0x01,                   // 1turn(val) 0push(bool)
        0x00,                   // dials
    };
    if (dialValues.size() != newDialValues.size()) {
        for (uint8_t i = 0; i < newDialValues.size(); i++)
        {
            dialReport[i + 4] = newDialValues[i];
        }
        isDialChanged = true;
    } else if (dialValues != newDialValues)
    {
        for (uint8_t i = 0; i < newDialValues.size(); i++)
        {
            dialReport[i + 4] = newDialValues[i] - dialValues[i];
        }
        isDialChanged = true;
    }
    if (isDialChanged) {
        dialValues = newDialValues;
        usb_hid.sendReport(1, dialReport, sizeof(dialReport));
        Serial.printf("Dial Changed: %d\n", (int8_t) dialReport[4]);
    }
}
*/

uint16_t get_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)buffer;
    (void)reqlen;

    Serial.printf("GET Report Type: %d, Report ID: %d\n", report_type, report_id);

    if (report_type == HID_REPORT_TYPE_FEATURE)
    {

        if (report_id == 0x05)
        {
            // version
            memmove(buffer, version, reqlen);
            return sizeof(buffer) * 8 - 1;
        } else if (report_id == 0x06)
        {
            // serial
            memmove(buffer, serial, reqlen);
            return sizeof(buffer) * 8 - 1;
        }
    }

    return 0;
}

uint8_t output_report_id = 0;
uint8_t output_report_buffer[OUTPUT_REPORT_LEN];
uint16_t output_report_written_len = 0;

// Invoked when received SET_REPORT control request or received data on OUT endpoint ( Report ID = 0, Type = 0 )
void pre_set_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    char str[110];
    sprintf(
        str,
        "SET Report Type: %d, Report ID: %d, len: %d, outputlen: %d, buf: [%02X, %02X, ...]",
        report_type, report_id, bufsize, output_report_written_len,
        buffer[0], buffer[1]);
    Serial.println(str);

    if (report_type == HID_REPORT_TYPE_INVALID && report_id == 0 &&
        (buffer[0] == OUTPUT_REPORT_ID || output_report_written_len > 0))
    {
        if (output_report_written_len == 0)
        {
            output_report_id = buffer[0];
            memmove(output_report_buffer, buffer + 1, bufsize - 1);
            output_report_written_len = bufsize - 1;
        } else {
            memmove(output_report_buffer + output_report_written_len, buffer, bufsize);
            output_report_written_len += bufsize;
        }

        if (output_report_written_len < OUTPUT_REPORT_LEN) {
            // report data is not complete yet
            return;
        }

        set_report_callback(output_report_id, HID_REPORT_TYPE_OUTPUT, output_report_buffer, output_report_written_len);
        output_report_written_len = 0;
        return;
    }

    // If it processes output report, but, another report type will come.
    if (output_report_written_len > 0 && report_type != HID_REPORT_TYPE_OUTPUT)
    {
        // reset output report buffer
        output_report_written_len = 0;
    }

    set_report_callback(report_id, report_type, buffer, bufsize);
}

uint8_t image_type = 0; // 0x07: key, 0x0C: touchscreen
uint8_t image_buffer[MAX_IMAGE_SIZE_BYTES];
uint16_t image_buffer_written_len = 0;

void set_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    if (report_type == HID_REPORT_TYPE_FEATURE)
    {
        if (report_id == 0x03 && buffer[0] == 0x02)
        {
            Serial.println("Reset");
        } else if (report_id == 0x03 && buffer[0] == 0x08)
        {
            Serial.println("Brightness");
        }
    }

    if (report_type == HID_REPORT_TYPE_OUTPUT)
    {
        Serial.printf("Output Report: %d, [%02X, %02X, %02X, %02X, %02X, %02X]\n", report_id, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

        // reset image buffer when another request comes
        if (report_id == 0x02 && image_buffer_written_len > 0 && buffer[0] != image_type)
        {
            image_buffer_written_len = 0;
        }

        if (report_id == 0x02 && buffer[0] == 0x07) {
            // key image
            image_type = buffer[0];
            uint8_t key_index = buffer[1];
            uint8_t is_last = buffer[2];
            uint16_t image_length = buffer[3] | (buffer[4] << 8);
            uint16_t page_number = buffer[5] | (buffer[6] << 8);

            memmove(image_buffer + image_buffer_written_len, buffer + 7, image_length);
            image_buffer_written_len += image_length;

            if (is_last) {
                lcd.draw_key_image(file_repository, key_index, image_buffer, image_buffer_written_len);
                image_buffer_written_len = 0;
            }
        } else if (report_id == 0x02 && buffer[0] == 0x0C) {
            // touchscreen image
            image_type = buffer[0];
            uint16_t x_pos = buffer[1] | (buffer[2] << 8);
            uint16_t y_pos = buffer[3] | (buffer[4] << 8);
            uint16_t width = buffer[5] | (buffer[6] << 8);
            uint16_t height = buffer[7] | (buffer[8] << 8);
            uint8_t is_last = buffer[9];
            uint16_t page_number = buffer[10] | (buffer[11] << 8);
            uint16_t image_length = buffer[12] | (buffer[13] << 8);
            // padding = buffer[14]

            memmove(image_buffer + image_buffer_written_len, buffer + 15, image_length);
            image_buffer_written_len += image_length;

            if (is_last) {
                // TODO: draw lcd
                image_buffer_written_len = 0;
            }
        } else if (report_id == 0x02 && buffer[0] == 0xFF)
        {
            rp2040.rebootToBootloader();
        }
    }
}
