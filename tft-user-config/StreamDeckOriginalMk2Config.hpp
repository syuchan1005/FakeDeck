/**
 * TFT configuration for the StreamDeckOriginalV2
 */

#define USER_SETUP_LOADED

#define ILI9488_DRIVER
#define TFT_MISO 16
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 20
#define TFT_DC 22
#define TFT_RST 21

#define TFT_LED 11

#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

#define TFT_ROTATION 1

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
#define TOUCH_X p.x, 3940, 220
#define TOUCH_Y p.y, 3870, 310
#define TOUCH_THRESHOLD 600