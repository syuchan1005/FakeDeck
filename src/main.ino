#include <Arduino.h>

#include <Adafruit_TinyUSB.h>
#include "./usb_descriptors.h"

#include "./input/buttons.cpp"

// TODO: Supports button array
Buttons buttons((uint8_t[]){3, 7, 11, 15, 16});

uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC()};

Adafruit_USBD_HID usb_hid;

void setup()
{
    buttons.init();

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

    uint8_t report[INPUT_REPORT_LEN] = {0x00, KEY_COUNT, 0x00};
    buttons.getButtons(report, 3);
    usb_hid.sendReport(1, report, sizeof(report));

    /*
    uint8_t touchscreenReport[INPUT_REPORT_LEN] = {
        0x02, 0x01, 0x00,    // touch, count(?), ?
        0x01,                // short
        U16_TO_U8S_LE(0x0A), // x
        U16_TO_U8S_LE(0x0A)  // y
    };
    See more: py\Lib\site-packages\StreamDeck\Devices\StreamDeckPlus.py#L356
    */

    /*
    uint8_t dialReport[INPUT_REPORT_LEN] = {
        0x03, DIAL_COUNT, 0x00, // dial, count, ?
        0x00, // 1turn(val) 0push(bool)
        0x01, // dials
        0x00,
        0x00,
        0x00,
    };
    See more: py\Lib\site-packages\StreamDeck\Devices\StreamDeckPlus.py#L371
    */
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
