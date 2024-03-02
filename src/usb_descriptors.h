#include <Adafruit_TinyUSB.h>

#define PEDAL 1
#define XL 2
#define PLUS 3

#define STREAM_DECK PLUS

#define DECK_USB_VID 0x0fd9
#define DECK_USB_MANUFACTURER "Elgato Systems GmbH"

#if STREAM_DECK == PEDAL

  #define DECK_USB_PID 0x0086
  #define DECK_USB_PRODUCT "Stream Deck Pedal"

  #define KEY_COUNT 3

  #define DECK_LED true

  #define DECK_VISUAL false

  // report_type, report_id, content
  /* Pedal
  Key: in, {00, 03, 00, ...keys, 00}
  Set LED: invalid, 0, {02, 0b, R, G, B}
  Serial: feature, 06
  Version: feature, 05
  */
  #define INPUT_REPORT_LEN KEY_COUNT + 4

#elif STREAM_DECK == XL

  #define DECK_USB_PID 0x006c
  #define DECK_USB_PRODUCT "Stream Deck XL"

  #define KEY_COUNT 32 // col 8 row 4

  #define DECK_LED false

  #define DECK_VISUAL true
  #define KEY_PIXEL_WIDTH 96
  #define KEY_PIXEL_HEIGHT 96
  #define KEY_IMAGE_FORMAT "JPEG"
  #define KEY_FLIP_HORIZONTAL true
  #define KEY_FLIP_VERTICAL true
  #define KEY_ROTATION 0
  #define IMAGE_REPORT_HEADER_LENGTH 8
  #define IMAGE_REPORT_LENGTH 1024

  /* XL
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
  #define DECK_USB_PRODUCT "Stream Deck +"

  #define KEY_COUNT 8 // col 4 row 2
  #define DIAL_COUNT 4

  #define DECK_LED false

  #define DECK_VISUAL true
  #define KEY_PIXEL_WIDTH 120
  #define KEY_PIXEL_HEIGHT 120
  #define KEY_IMAGE_FORMAT "JPEG"
  #define KEY_FLIP_HORIZONTAL false
  #define KEY_FLIP_VERTICAL false
  #define KEY_ROTATION 0

  #define DECK_TOUCH true
  #define TOUCHSCREEN_PIXEL_HEIGHT 100
  #define TOUCHSCREEN_PIXEL_WIDTH 800
  #define TOUCHSCREEN_IMAGE_FORMAT "JPEG"
  #define TOUCHSCREEN_FLIP_HORIZONTAL false
  #define TOUCHSCREEN_FLIP_VERTICAL false
  #define TOUCHSCREEN_ROTATION 0

  #define IMAGE_REPORT_HEADER_LENGTH 8
  #define IMAGE_REPORT_LCD_HEADER_LENGTH 16
  #define IMAGE_REPORT_LENGTH 1024

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
    HID_REPORT_COUNT_N ( 1023, 2), \
    HID_REPORT_ID    ( 2 ) \
    HID_USAGE ( HID_USAGE_CONSUMER_CONTROL ), \
    HID_OUTPUT       ( OUTPUT_REPORT_FLAGS ) ,\
    \
    /* keys, touchscreen */ \
    HID_REPORT_COUNT ( INPUT_REPORT_LEN ), \
    HID_REPORT_ID    ( 0x01 ) \
    HID_USAGE ( HID_USAGE_CONSUMER_CONTROL ), \
    HID_INPUT        ( INPUT_REPORT_FLAGS ), \
    \
    /* TODO: dials */ \
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
