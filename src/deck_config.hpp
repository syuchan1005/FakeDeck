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
#define KEY_ORDER_REVERSE

// #define DIAL_COUNT 0

// #define DECK_TOUCH
// #define TOUCHSCREEN_PIXEL_WIDTH 0
// #define TOUCHSCREEN_PIXEL_HEIGHT 0

#define INPUT_REPORT_LEN KEY_COUNT_COL *KEY_COUNT_ROW + 4

#define KEY_H_GAP(w) ((w) - (KEY_IMAGE_SIZE * KEY_COUNT_COL)) / (KEY_COUNT_COL + 1)
#define KEY_V_GAP(h) ((h) - (KEY_IMAGE_SIZE * KEY_COUNT_ROW)) / (KEY_COUNT_ROW + 1)
#define KEY_OFFSET_X(w) KEY_H_GAP(w)
#define KEY_OFFSET_Y(h) KEY_V_GAP(h)

#elif DECK == PLUS

#define DECK_USB_PID 0x0084
#define DECK_USB_PRODUCT "Streamdeck +"

#define KEY_COUNT_COL 4
#define KEY_COUNT_ROW 2
#define KEY_IMAGE_SIZE 120

#define DIAL_COUNT 4

#define DECK_TOUCH
#define TOUCHSCREEN_PIXEL_WIDTH 200
#define TOUCHSCREEN_PIXEL_HEIGHT 100

#define INPUT_REPORT_LEN 14

#define KEY_H_GAP(w) ((w) - (KEY_IMAGE_SIZE * KEY_COUNT_COL)) / (KEY_COUNT_COL + 1)
#define KEY_V_GAP(h) ((h) - (KEY_IMAGE_SIZE * KEY_COUNT_ROW + TOUCHSCREEN_PIXEL_HEIGHT)) / (KEY_COUNT_ROW + 1)
#define KEY_OFFSET_X(w) KEY_H_GAP(w)
#define KEY_OFFSET_Y(h) (KEY_V_GAP(h) / 2)
#define TOUCH_OFFSET_Y(h) h - TOUCHSCREEN_PIXEL_HEIGHT

#else
#error "Invalid DECK value"
#endif

#define KEY_X(w, keyIndex) KEY_OFFSET_X(w) + (KEY_IMAGE_SIZE + KEY_H_GAP(w)) * (keyIndex % KEY_COUNT_COL)
#define KEY_Y(h, keyIndex) KEY_OFFSET_Y(h) + (KEY_IMAGE_SIZE + KEY_V_GAP(h)) * (keyIndex / KEY_COUNT_COL)

#endif // DECK_CONFIG_HPP