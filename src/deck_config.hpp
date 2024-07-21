#ifndef DECK_CONFIG_HPP
#define DECK_CONFIG_HPP

#define ORIGINAL_V2 1
#define PLUS 2

#if DECK == ORIGINAL_V2

  #define DECK_USB_PID 0x006d
  #define DECK_USB_PRODUCT "Streamdeck"

  #define KEY_COUNT_COL 5
  #define KEY_COUNT_ROW 3
  #define KEY_IMAGE_SIZE 72

  // #define DIAL_COUNT 0

  // #define DECK_TOUCH
  // #define TOUCHSCREEN_PIXEL_WIDTH 0
  // #define TOUCHSCREEN_PIXEL_HEIGHT 0

  #define INPUT_REPORT_LEN KEY_COUNT_COL * KEY_COUNT_ROW + 4

#elif DECK == PLUS

  #define DECK_USB_PID 0x0084
  #define DECK_USB_PRODUCT "Streamdeck +"

  #define KEY_COUNT_COL 4
  #define KEY_COUNT_ROW 2
  #define KEY_IMAGE_SIZE 120

  #define DIAL_COUNT 4

  #define DECK_TOUCH
  #define TOUCHSCREEN_PIXEL_WIDTH 800
  #define TOUCHSCREEN_PIXEL_HEIGHT 100

  #define INPUT_REPORT_LEN 14

#else
  #error "Invalid DECK value"
#endif

#endif // DECK_CONFIG_HPP