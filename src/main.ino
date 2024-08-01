#include <memory>
#include <algorithm>
#include <Adafruit_TinyUSB.h>
#include "./usb_descriptors.hpp"
#include "./input/LCD.hpp"
#include "./input/Encoder.hpp"
#include "./FileRepository.hpp"

Adafruit_USBD_HID usb_hid;
uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC()};

Input::Display::LCD lcd;
Input::Encoder::Encoder encoder;
FileRepository file_repository;

// 6.2.0.18816
uint8_t version[20] = {0x0C, 0xD9, 0x4B, 0x72, 0xE0, 0x36, 0x2E, 0x32, 0x2E, 0x30, 0x2E, 0x31, 0x38, 0x38, 0x31, 0x36, 0x00, 0x00, 0x00};
// ZZZZZZZZZZZZZ
uint8_t serial[20] = {0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x00, 0x00};

queue_t report_packet_queue;
struct report_packet_t
{
    uint8_t report_id;
    hid_report_type_t report_type;
    uint8_t buffer[OUTPUT_REPORT_LEN];
    uint16_t bufsize;
};

queue_t brightness_queue;
queue_t display_event_queue;
queue_t encoder_event_queue;

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

    queue_init(&report_packet_queue, sizeof(report_packet_t), 20);
    queue_init(&brightness_queue, sizeof(uint8_t), 2);
    queue_init(&display_event_queue, sizeof(Input::Display::Event::Event), 4);
    queue_init(&encoder_event_queue, sizeof(Input::Encoder::Event::Event), 4);
}

// WARNING: We should not use `wait` function (e.g. `delay`) in this function. TinyUSB's tud_task should be called in the loop. And it's needed to be called very frequently.
void loop()
{
    Input::Display::Event::Event event = Input::Display::Event::NONE_OBJ;
    if (queue_try_remove(&display_event_queue, &event))
    {
        Serial.printf("Event { type: %d, keyIndex: %d, x: %d, y: %d, x_out: %d, y_out: %d }\n", event.type, event.keyIndex, event.x, event.y, event.x_out, event.y_out);
        switch (event.type)
        {
        case Input::Display::Event::NONE:
        {
            Serial.println("No Key Pressed");
            uint8_t report[INPUT_REPORT_LEN] = {0, KEY_COUNT, 0};
            usb_hid.sendReport(1, report, sizeof(report));
            break;
        }
        case Input::Display::Event::KEY_PRESSED:
        {
            Serial.printf("Pressed Key: %d\n", event.keyIndex);
            uint8_t report[INPUT_REPORT_LEN] = {0, KEY_COUNT, 0};
            report[event.keyIndex + 3] = 1;
            usb_hid.sendReport(1, report, sizeof(report));
            break;
        }
#if defined(DECK_TOUCH)
        case Input::Display::Event::TOUCH_PRESSED_SHORT:
        {
            uint8_t touchReport[INPUT_REPORT_LEN] =
                {0x02, DIAL_COUNT, 0x00,
                 0x01, 0x00, U16_TO_U8S_LE(event.x), U16_TO_U8S_LE(event.y), 0x00};
            usb_hid.sendReport(1, touchReport, sizeof(touchReport));
            break;
        }
        case Input::Display::Event::TOUCH_PRESSED_LONG:
        {
            uint8_t touchReport[INPUT_REPORT_LEN] =
                {0x02, DIAL_COUNT, 0x00,
                 0x02, 0x00, U16_TO_U8S_LE(event.x), U16_TO_U8S_LE(event.y), 0x00};
            usb_hid.sendReport(1, touchReport, sizeof(touchReport));
            break;
        }
        case Input::Display::Event::TOUCH_DRAG:
        {
            uint8_t touchReport[INPUT_REPORT_LEN] =
                {0x02, DIAL_COUNT, 0x00,
                 0x03, 0x00, U16_TO_U8S_LE(event.x), U16_TO_U8S_LE(event.y), U16_TO_U8S_LE(event.x_out), U16_TO_U8S_LE(event.y_out), 0x00};
            usb_hid.sendReport(1, touchReport, sizeof(touchReport));
            break;
        }
#endif
        default:
            Serial.printf("Unknown Display Event: %d\n", event.type);
            break;
        }
    }
#if defined(DECK_TOUCH)
    Input::Encoder::Event::Event encoderEvent = Input::Encoder::Event::Event(Input::Encoder::Event::EventType::NONE, 0);
    if (queue_try_remove(&encoder_event_queue, &encoderEvent))
    {
        Serial.printf("Encoder Event { type: %d, value: ", encoderEvent.type);
        Serial.print(encoderEvent.value, BIN);
        Serial.println(" }");
        switch (encoderEvent.type)
        {
        case Input::Encoder::Event::EventType::TURN:
        {
            uint8_t report[INPUT_REPORT_LEN] =
                {0x03, DIAL_COUNT + 1, 0x00,
                 0x01,
                 (uint8_t)make_encoder_turn_report(encoderEvent.value),
                 (uint8_t)make_encoder_turn_report(encoderEvent.value >> 2),
                 (uint8_t)make_encoder_turn_report(encoderEvent.value >> 4),
                 (uint8_t)make_encoder_turn_report(encoderEvent.value >> 6),
                 0x00};
            usb_hid.sendReport(1, report, sizeof(report));
            break;
        }
        case Input::Encoder::Event::EventType::PRESS:
        {
            uint8_t report[INPUT_REPORT_LEN] =
                {0x03, DIAL_COUNT + 1, 0x00,
                0x00,
                encoderEvent.value & 1,
                encoderEvent.value & (1 << 1),
                encoderEvent.value & (1 << 2),
                encoderEvent.value & (1 << 3),
                0x00};
            Serial.print("report:");
            for (int i = 0; i < INPUT_REPORT_LEN; i++)
            {
                Serial.print(report[i], HEX);
                Serial.print(" ");
            }
            Serial.println();

            usb_hid.sendReport(1, report, sizeof(report));
            break;
        }

        default:
            Serial.printf("Unknown Encoder Event: %d\n", encoderEvent.type);
            break;
        }
    }
#endif
}

uint8_t reverse(uint8_t num)
{
    uint8_t reverse_num = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if ((num & (1 << i)))
            reverse_num |= 1 << (7 - i);
    }
    return reverse_num;
}

int8_t make_encoder_turn_report(uint8_t turn)
{
    switch (turn & 0x03)
    {
    case 1:
        return -1; // turn left
    case 2:
        return 1; // turn right
    default:
    case 3:
        return 0; // no turn
    }
}

void setup1()
{
    lcd.init();
    file_repository.init();
    lcd.calibrate(file_repository);

    encoder.init();
}

void loop1()
{
    report_packet_t packet;
    if (queue_try_remove(&report_packet_queue, &packet))
    {
        set_report_callback(packet.report_id, packet.report_type, packet.buffer, packet.bufsize);
    }

    uint8_t brightness;
    if (queue_try_remove(&brightness_queue, &brightness))
    {
        lcd.set_brightness(brightness);
    }

    maybe_send_event();
}

Input::Display::Event::Event previous_event = Input::Display::Event::NONE_OBJ;
uint32_t previous_executed_event_millis = 0;
void maybe_send_event()
{
    uint32_t current_millis = millis();
    if (current_millis - previous_executed_event_millis < 10)
    {
        return;
    }
    previous_executed_event_millis = current_millis;
    Input::Display::Event::Event event = lcd.get_event();
    if (previous_event.type != event.type)
    {
        queue_add_blocking(&display_event_queue, &event);
        if (event.type == Input::Display::Event::EventType::KEY_PRESSED)
            previous_event = event;
        else
            previous_event = Input::Display::Event::NONE_OBJ;
    }

    maybe_send_encoder_event();
}

Input::Encoder::Event::EventDataHolder previous_encoder_event_data_holder = Input::Encoder::Event::EventDataHolder(0, 0);
void maybe_send_encoder_event()
{
    Input::Encoder::Event::EventDataHolder eventDataHolder = encoder.get_event();
    if (previous_encoder_event_data_holder.turn != eventDataHolder.turn)
    {
        Input::Encoder::Event::Event event = Input::Encoder::Event::Event(Input::Encoder::Event::EventType::TURN, eventDataHolder.turn);
        queue_add_blocking(&encoder_event_queue, &event);
    }
    if (previous_encoder_event_data_holder.press != eventDataHolder.press)
    {
        Input::Encoder::Event::Event event = Input::Encoder::Event::Event(Input::Encoder::Event::EventType::PRESS, eventDataHolder.press);
        queue_add_blocking(&encoder_event_queue, &event);
    }
    previous_encoder_event_data_holder = eventDataHolder;
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

        emit_report_packet(output_report_id, HID_REPORT_TYPE_OUTPUT, output_report_buffer, output_report_written_len);
        output_report_written_len = 0;
        return;
    }

    // If it processes output report, but, another report type will come.
    if (output_report_written_len > 0 && report_type != HID_REPORT_TYPE_OUTPUT)
    {
        // reset output report buffer
        output_report_written_len = 0;
    }

    emit_report_packet(report_id, report_type, buffer, bufsize);
}

void emit_report_packet(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    report_packet_t packet;
    packet.report_id = report_id;
    packet.report_type = report_type;
    std::copy_n(buffer, bufsize, packet.buffer);
    packet.bufsize = bufsize;
    queue_try_add(&report_packet_queue, &packet);
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
                Serial.printf("Draw image: %d\n", key_index);
                lcd.draw_key_image(key_index, image_buffer, image_buffer_written_len);
                image_buffer_written_len = 0;
            }
        }
        else if (report_id == 0x02 && buffer[0] == 0x0C)
        {
            // touchscreen image
            image_type = buffer[0];
            uint16_t x_pos = buffer[1] | (buffer[2] << 8);
            uint16_t y_pos = buffer[3] | (buffer[4] << 8);
            // uint16_t width = buffer[5] | (buffer[6] << 8);
            // uint16_t height = buffer[7] | (buffer[8] << 8);
            uint8_t is_last = buffer[9];
            uint16_t page_number = buffer[10] | (buffer[11] << 8);
            uint16_t image_length = buffer[12] | (buffer[13] << 8);
            // padding = buffer[14]

            std::copy_n(buffer + 15, image_length, image_buffer + image_buffer_written_len);
            image_buffer_written_len += image_length;

            if (is_last)
            {
                lcd.draw_touch_image(x_pos, y_pos, image_buffer, image_buffer_written_len);
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
