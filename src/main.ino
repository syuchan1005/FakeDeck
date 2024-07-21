#include <memory>
#include <algorithm>
#include <Adafruit_TinyUSB.h>
#include "./usb_descriptors.hpp"
#include "./input/LCD.hpp"
#include "./FileRepository.hpp"

Adafruit_USBD_HID usb_hid;
uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC()};

Input::LCD lcd;
FileRepository file_repository;

// 6.2.0.18816
uint8_t version[20] = {0x0C, 0xD9, 0x4B, 0x72, 0xE0, 0x36, 0x2E, 0x32, 0x2E, 0x30, 0x2E, 0x31, 0x38, 0x38, 0x31, 0x36, 0x00, 0x00, 0x00};
// ZZZZZZZZZZZZZ
uint8_t serial[20] = {0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x00, 0x00};

queue_t draw_image_queue;
struct draw_image_t
{
public:
    uint8_t key_index;
    uint8_t image_data[MAX_IMAGE_SIZE_BYTES];
    uint16_t image_length;
};

queue_t brightness_queue;
queue_t pressed_key_queue;

void setup()
{
    Serial.begin(115200);

    TinyUSBDevice.setManufacturerDescriptor(DECK_USB_MANUFACTURER);
    TinyUSBDevice.setProductDescriptor(DECK_USB_PRODUCT);
    TinyUSBDevice.setID(DECK_USB_VID, DECK_USB_PID);

    usb_hid.enableOutEndpoint(true);
    usb_hid.setPollInterval(2);
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid.setReportCallback(get_report_callback, pre_set_report_callback);
    usb_hid.begin();

    queue_init(&draw_image_queue, sizeof(draw_image_t), 2);
    queue_init(&brightness_queue, sizeof(uint8_t), 2);
    queue_init(&pressed_key_queue, sizeof(uint8_t), 2);
}

// WARNING: We should not use `wait` function (e.g. `delay`) in this function. TinyUSB's tud_task should be called in the loop. And it's needed to be called very frequently.
void loop()
{
    uint8_t pressed_key = Input::NO_KEY_PRESSED;
    if (queue_try_remove(&pressed_key_queue, &pressed_key))
    {
        if (pressed_key != Input::NO_KEY_PRESSED)
        {
            Serial.printf("Pressed Key: %d\n", pressed_key);
            uint8_t report[INPUT_REPORT_LEN] = {0, KEY_COUNT, 0};
            report[pressed_key + 3] = 1;
            usb_hid.sendReport(1, report, sizeof(report));
        }
        else
        {
            Serial.println("No Key Pressed");
            uint8_t report[INPUT_REPORT_LEN] = {0, KEY_COUNT, 0};
            usb_hid.sendReport(1, report, sizeof(report));
        }
    }
}

void setup1()
{
    lcd.init();
    file_repository.init();
    lcd.calibrate(file_repository);
}

void loop1()
{
    draw_image_t entry;
    if (queue_try_remove(&draw_image_queue, &entry))
    {
        Serial.printf("Draw Key Image: %d\n", entry.key_index);
        lcd.draw_key_image(entry.key_index, entry.image_data, entry.image_length);
    }
    uint8_t brightness;
    if (queue_try_remove(&brightness_queue, &brightness))
    {
        lcd.set_brightness(brightness);
    }

    maybe_send_pressed_key();
}

uint8_t previous_pressed_key = Input::NO_KEY_PRESSED;
uint32_t previous_executed_millis = 0;
void maybe_send_pressed_key()
{
    uint32_t current_millis = millis();
    if (current_millis - previous_executed_millis < 10)
    {
        return;
    }
    previous_executed_millis = current_millis;

    uint8_t pressed_key = lcd.get_pressed_button();
    bool shouldSendPressedReport = false;
    if (pressed_key != previous_pressed_key)
    {
        queue_add_blocking(&pressed_key_queue, &pressed_key);
    }
    previous_pressed_key = pressed_key;
}

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
            std::copy_n(version, reqlen, buffer);
            return sizeof(buffer) * 8 - 1;
        }
        else if (report_id == 0x06)
        {
            // serial
            std::copy_n(serial, reqlen, buffer);
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
        "SET Report Type: %d, Report ID: %d, len: %d, outputlen: %04d, buf: [%02X, %02X, ...]",
        report_type, report_id, bufsize, output_report_written_len,
        buffer[0], buffer[1]);
    Serial.println(str);

    if (report_type == HID_REPORT_TYPE_INVALID && report_id == 0 &&
        (buffer[0] == OUTPUT_REPORT_ID || output_report_written_len > 0))
    {
        if (output_report_written_len == 0)
        {
            output_report_id = buffer[0];
            std::copy_n(buffer + 1, bufsize - 1, output_report_buffer);
            output_report_written_len = bufsize - 1;
        }
        else
        {
            std::copy_n(buffer, bufsize, output_report_buffer + output_report_written_len);
            output_report_written_len += bufsize;
        }

        if (output_report_written_len < OUTPUT_REPORT_LEN)
        {
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
        }
        else if (report_id == 0x03 && buffer[0] == 0x08)
        {
            queue_add_blocking(&brightness_queue, &buffer[1]);
        }
    }

    if (report_type == HID_REPORT_TYPE_OUTPUT)
    {
        Serial.printf("Output Report: %d, %d, [%02X, %02X, %02X, %02X, %02X, %02X]\n", report_id, bufsize, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

        // reset image buffer when another request comes
        if (report_id == 0x02 && image_buffer_written_len > 0 && buffer[0] != image_type)
        {
            image_buffer_written_len = 0;
        }

        if (report_id == 0x02 && buffer[0] == 0x07)
        {
            // key image
            image_type = buffer[0];
            uint8_t key_index = buffer[1];
            uint8_t is_last = buffer[2];
            uint16_t image_length = buffer[3] | (buffer[4] << 8);
            uint16_t page_number = buffer[5] | (buffer[6] << 8);

            std::copy_n(buffer + 7, image_length, image_buffer + image_buffer_written_len);
            image_buffer_written_len += image_length;

            if (is_last)
            {
                draw_image_t entry;
                entry.key_index = key_index;
                entry.image_length = image_buffer_written_len;
                std::copy_n(image_buffer, image_buffer_written_len, entry.image_data);
                Serial.printf("Add Draw Image Queue: %d\n", key_index);
                queue_add_blocking(&draw_image_queue, &entry);
                image_buffer_written_len = 0;
            }
        }
        else if (report_id == 0x02 && buffer[0] == 0x0C)
        {
            // touchscreen image
            image_type = buffer[0];
            uint16_t x_pos = buffer[1] | (buffer[2] << 8);
            uint16_t y_pos = buffer[3] | (buffer[4] << 8);
            uint16_t width = buffer[5] | (buffer[6] << 8);
            uint16_t height = buffer[7] | (buffer[8] << 8);
            uint8_t is_last = buffer[9];
            uint16_t page_number = buffer[10] | (buffer[11] << 8);
            uint16_t image_length = buffer[12] | (buffer[13] << 8);
            // padding = buffer[14]testesttestest

            std::copy_n(buffer + 15, image_length, image_buffer + image_buffer_written_len);
            image_buffer_written_len += image_length;

            if (is_last)
            {
                // TODO: draw lcd
                image_buffer_written_len = 0;
            }
        }
        else if (report_id == 0x02 && buffer[0] == 0xFF)
        {
            rp2040.rebootToBootloader();
        }
        else if (report_id == 0x02 && buffer[0] == 0xFE)
        {
            rp2040.reboot();
        }
    }
}
