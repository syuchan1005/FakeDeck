/**
 * TFT configuration for the StreamDeckPlus
 */

#define USER_SETUP_LOADED

#define TFT_PARALLEL_8_BIT

#define SSD1963_800_DRIVER

#define TFT_RGB_ORDER TFT_RGB

#define TFT_ROTATION 3

// #define TFT_CS // to GND
// #define TFT_RD // to 3V3
#define TFT_DC 11
#define TFT_RST 20
#define TFT_WR 10

// PIO requires these to be sequentially increasing
#define TFT_D0 2
#define TFT_D1 3
#define TFT_D2 4
#define TFT_D3 5
#define TFT_D4 6
#define TFT_D5 7
#define TFT_D6 8
#define TFT_D7 9

#define TFT_LED 21

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define USE_ORIGINAL_TOUCH
#define TFT_MISO2 12
#define TFT_MOSI2 15
#define TFT_SCLK2 14
#define TOUCH_CS2 13
#define TOUCH_X p.y, 3700, 250
#define TOUCH_Y p.x, 450, 3600
