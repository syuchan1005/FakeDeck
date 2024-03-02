#include <Arduino.h>

#include <Adafruit_TinyUSB.h>
#include "./usb_descriptors.h"

#include "./input/buttons.cpp"
#include "./input/encoder.h"

// TODO: Supports button array
Buttons buttons((uint8_t[]){3, 7, 11, 15, 16});
Encoders encoders((uint8_t[]){17, 18});

uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC()};

Adafruit_USBD_HID usb_hid;

#if STREAM_DECK == PEDAL || STREAM_DECK == XL
// 2.0.2.8
uint8_t version[15] = {0x0C, 0xD9, 0x4B, 0x72, 0xE0, 0x32, 0x2E, 0x30, 0x2E, 0x32, 0x2E, 0x38, 0x00, 0x00, 0x00};
// ZZZZZZZZZZZZZ
uint8_t serial[15] = {0x0C, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x00, 0x00};
#else
// 6.2.0.18816
uint8_t version[20] = {0x0C, 0xD9, 0x4B, 0x72, 0xE0, 0x36, 0x2E, 0x32, 0x2E, 0x30, 0x2E, 0x31, 0x38, 0x38, 0x31, 0x36, 0x00, 0x00, 0x00};
// ZZZZZZZZZZZZZ
uint8_t serial[20] = {0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x00, 0x00};
#endif

void setup()
{
    buttons.init();
    encoders.init();

    TinyUSBDevice.setManufacturerDescriptor(DECK_USB_MANUFACTURER);
    TinyUSBDevice.setProductDescriptor(DECK_USB_PRODUCT);
    TinyUSBDevice.setID(DECK_USB_VID, DECK_USB_PID);

    usb_hid.enableOutEndpoint(true);
    usb_hid.setPollInterval(2);
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid.setReportCallback(get_report_callback, set_report_callback);
    usb_hid.begin();

    Serial.begin(115200);

    // wait until device mounted
    while (!TinyUSBDevice.mounted())
        delay(1);
}

void loop()
{
    delay(100);

    uint8_t report[INPUT_REPORT_LEN] = {0x00, KEY_COUNT, 0x00}; // key, count, ?
    buttons.getButtons(report, 3);
    usb_hid.sendReport(1, report, sizeof(report));

    maybeSendDialReport();

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

uint16_t get_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)buffer;
    (void)reqlen;

    char str[80];
    sprintf(str, "GET Report Type: %d, Report ID: %d", report_type, report_id);
    Serial.println(str);

    if (report_type == HID_REPORT_TYPE_FEATURE)
    {

        if (report_id == 0x05)
        {
            // version
            memcpy(buffer, version, reqlen);
            return sizeof(buffer) * 8 - 1;
        }
        if (report_id == 0x06)
        {
            // serial
            memcpy(buffer, serial, reqlen);
            return sizeof(buffer) * 8 - 1;
        }
    }

    return 0;
}

// Invoked when received SET_REPORT control request or received data on OUT endpoint ( Report ID = 0, Type = 0 )
void set_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    char str[110];
    sprintf(
        str,
        "SET Report Type: %d, Report ID: %d, len: %d, buf: [%02X, %02X, ...]",
        report_type, report_id, bufsize,
        buffer[0], buffer[1]);
    Serial.println(str);

    // TODO: For over 64 bytes packet, TinyUSB stack will call set_report_callback multiple times.
    //       So, we should store the report data and process it when complete.
    if (report_type == HID_REPORT_TYPE_INVALID && report_id == 0)
    {
        // LED Color
        if (buffer[0] == 0x02 && buffer[1] == 0x0b)
        {
            sprintf(str, "RGB: %02X%02X%02X", buffer[2], buffer[3], buffer[4]);
            Serial.println(str);
        }
    }

    if (report_type == HID_REPORT_TYPE_FEATURE)
    {
        if (report_id == 0x03 && buffer[0] == 0x02)
        {
            // reset
        }
        if (report_id == 0x03 && buffer[0] == 0x08)
        {
            // brightness
        }
    }

    if (report_type == HID_REPORT_TYPE_OUTPUT)
    {
    }
}
