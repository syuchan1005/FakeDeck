#ifndef USB_DESCRIPTORS_HPP
#define USB_DESCRIPTORS_HPP

#include <Adafruit_TinyUSB.h>

#define ORIGINAL_V2 1
#define PLUS 2

#define STREAM_DECK ORIGINAL_V2

#define DECK_USB_VID 0x0fd9
#define DECK_USB_MANUFACTURER "Elgato Systems GmbH"

#if STREAM_DECK == ORIGINAL_V2

  #define DECK_USB_PID 0x006d
  #define DECK_USB_PRODUCT "Streamdeck"

  #define KEY_COUNT_COL 5
  #define KEY_COUNT_ROW 3
  #define KEY_IMAGE_SIZE 72

  #define TOUCHSCREEN_PIXEL_WIDTH 0
  #define TOUCHSCREEN_PIXEL_HEIGHT 0

  /* Original v2
  Key: in,
  reset_key: out, 02, 00 ...
  reset: feature, 03, 02
  brightness: feature, 03, 08, VALUE
  serial: feature, 06
  version: faeture, 05
  key_image: out, 02, 07, KEY, IS_LAST, IMAGE_LEN_LOW, IMAGE_LEN_HIGH, ...
  */
  #define INPUT_REPORT_LEN KEY_COUNT + 4
#elif STREAM_DECK == PLUS

  #define DECK_USB_PID 0x0084
  #define DECK_USB_PRODUCT "Streamdeck +"

  #define KEY_COUNT_COL 4
  #define KEY_COUNT_ROW 2
  #define KEY_IMAGE_SIZE 120

  #define DIAL_COUNT 4

  #define DECK_TOUCH
  #define TOUCHSCREEN_PIXEL_WIDTH 800
  #define TOUCHSCREEN_PIXEL_HEIGHT 100

  /* Plus
  Key: in, 00 ...
  TouchScreen: in, 02 ...
  Dial: in, 03 ...
  reset_key: out, 02, 00 ...
  reset: feature, 03, 02
  brightness: feature, 03, 08, VALUE
  serial: feature, 06
  version: faeture, 05
  key_image: out, 02, 07, KEY, IS_LAST, IMAGE_LEN_LOW, IMAGE_LEN_HIGH, ...
  touchscreen_image: out, 02, 0c ...
  */
  #define INPUT_REPORT_LEN 14

#endif

// Common
#define OUTPUT_REPORT_ID  2
#define OUTPUT_REPORT_LEN 1023

#define KEY_IMAGE_SIZE_BYTES (KEY_IMAGE_SIZE * KEY_IMAGE_SIZE * 3)
#define TOUCHSCREEN_SIZE_BYTES (TOUCHSCREEN_PIXEL_WIDTH * TOUCHSCREEN_PIXEL_HEIGHT * 3)
#if KEY_IMAGE_SIZE_BYTES > TOUCHSCREEN_SIZE_BYTES
  #define MAX_IMAGE_SIZE_BYTES KEY_IMAGE_SIZE_BYTES
#else
  #define MAX_IMAGE_SIZE_BYTES TOUCHSCREEN_SIZE_BYTES
#endif

#define KEY_COUNT KEY_COUNT_COL * KEY_COUNT_ROW

// 6.2.0.18816
uint8_t version[20] = {0x0C, 0xD9, 0x4B, 0x72, 0xE0, 0x36, 0x2E, 0x32, 0x2E, 0x30, 0x2E, 0x31, 0x38, 0x38, 0x31, 0x36, 0x00, 0x00, 0x00};
// ZZZZZZZZZZZZZ
uint8_t serial[20] = {0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x00, 0x00};

// Default
#ifndef DIAL_COUNT
  #define DIAL_COUNT 0
#endif

#define INPUT_REPORT_FLAGS   HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_WRAP_NO | HID_LINEAR | HID_PREFERRED_STATE | HID_NO_NULL_POSITION
#define OUTPUT_REPORT_FLAGS  HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_WRAP_NO | HID_LINEAR | HID_PREFERRED_STATE | HID_NO_NULL_POSITION | HID_NON_VOLATILE
#define FEATURE_REPORT_FLAGS HID_DATA | HID_ARRAY    | HID_RELATIVE | HID_WRAP_NO | HID_LINEAR | HID_PREFERRED_STATE | HID_NO_NULL_POSITION | HID_NON_VOLATILE
#define REPORT_FIELD()  HID_LOGICAL_MIN  ( 0 ), HID_LOGICAL_MAX_N  ( 0xFF, 2 ), HID_REPORT_SIZE  ( 8 ), 

#define TUD_HID_REPORT_DESC() \
HID_USAGE_PAGE ( HID_USAGE_PAGE_CONSUMER    ), \
HID_USAGE      ( HID_USAGE_CONSUMER_CONTROL ), \
HID_COLLECTION ( HID_COLLECTION_APPLICATION ), \
  /** Uses Vendor Page for whole collection unless redefined. **/ \
  HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),  \
  REPORT_FIELD() \
    /* reset, image, etc... */ \
    HID_REPORT_COUNT_N ( OUTPUT_REPORT_LEN, 2), \
    HID_REPORT_ID    ( OUTPUT_REPORT_ID ) \
    HID_USAGE ( HID_USAGE_CONSUMER_CONTROL ), \
    HID_OUTPUT       ( OUTPUT_REPORT_FLAGS ) ,\
    \
    /* keys, touchscreen, dials */ \
    HID_REPORT_COUNT ( INPUT_REPORT_LEN ), \
    HID_REPORT_ID    ( 0x01 ) \
    HID_USAGE ( HID_USAGE_CONSUMER_CONTROL ), \
    HID_INPUT        ( INPUT_REPORT_FLAGS ), \
    \
    /* reset */ \
    HID_REPORT_COUNT ( 31 ), \
    HID_REPORT_ID    (0x03) \
    HID_USAGE ( HID_USAGE_CONSUMER_CONTROL ), \
    HID_FEATURE      ( FEATURE_REPORT_FLAGS ), \
    \
    /* version */ \
    HID_REPORT_COUNT ( 31 ), \
    HID_REPORT_ID    (0x05) \
    HID_USAGE ( HID_USAGE_CONSUMER_CONTROL ), \
    HID_FEATURE      ( FEATURE_REPORT_FLAGS ), \
    \
    /* serial */ \
    HID_REPORT_COUNT ( 31 ), \
    HID_REPORT_ID    (0x06) \
    HID_USAGE ( HID_USAGE_CONSUMER_CONTROL ), \
    HID_FEATURE      ( FEATURE_REPORT_FLAGS ), \
HID_COLLECTION_END

#endif // USB_DESCRIPTORS_HPP
