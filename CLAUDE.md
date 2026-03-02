# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Fakedeck is an embedded C++ project that implements a DIY Stream Deck using Raspberry Pi Pico/Pico W. The project supports two hardware variants:
- **StreamDeck Mk.2**: Uses Raspberry Pi Pico with ILI9488 TFT display (5x3 key grid)
- **StreamDeck Plus**: Uses Raspberry Pi Pico W with SSD1963 TFT display (4x2 key grid + 4 rotary encoders + touchscreen)

The device emulates a USB HID device and communicates via USB to display images on keys and report button presses, encoder turns, and touch events.

## Build System

This project uses **PlatformIO** (not Arduino IDE). All builds must be done through PlatformIO.

### Common Commands

**Building firmware:**
```bash
# Build for StreamDeck Mk.2
pio run -e pico_deckMk2

# Build for StreamDeck Plus
pio run -e pico_deckPlus

# Build AVR ISP programmer (Pico W only)
pio run -e pico_avrisp_wifi

# Build ATtiny2313 firmware (for StreamDeck Plus rotary encoders)
pio run -e attiny2313
```

**Uploading firmware:**
```bash
# Upload to Pico (enter BOOTSEL mode first)
pio run -e pico_deckMk2 -t upload

# Upload ATtiny2313 via WiFi (requires config.ini)
pio run -e attiny2313 -t upload
```

**Cleaning:**
```bash
pio run -t clean
```

### Configuration

Before building StreamDeck Plus projects, copy `config.sample.ini` to `config.ini` and configure WiFi settings:
```ini
[config_avrisp]
build_flags =
    -D WIFI_SSID=\"your-ssid\"
    -D WIFI_PW=\"your-password\"

[config_avr]
upload_port = net:<your-pico-ip>:328
```

## Architecture

### Multi-Core Architecture

The firmware uses RP2040's dual-core architecture:
- **Core 0 (`setup()`/`loop()`)**: Handles USB HID communication, processes queued events, and sends HID reports to host
- **Core 1 (`setup1()`/`loop1()`)**: Handles hardware I/O (LCD, touchscreen, encoders), processes incoming USB reports, and manages display updates

Communication between cores uses RP2040's `queue_t` FreeRTOS queues:
- `report_packet_queue`: Core 0 → Core 1 (incoming USB reports)
- `brightness_queue`: Core 0 → Core 1 (brightness control)
- `display_event_queue`: Core 1 → Core 0 (button/touch events)
- `encoder_event_queue`: Core 1 → Core 0 (rotary encoder events)

### Project Structure

```
src/
├── main.ino              # Main firmware with dual-core setup and USB HID handling
├── deck_config.hpp       # Hardware variant configurations (Mk.2 vs Plus)
├── usb_descriptors.hpp   # USB HID descriptors and report definitions
├── FileRepository.hpp    # LittleFS-based file storage for calibration data
├── input/
│   ├── LCD.hpp          # TFT display, touch handling, JPEG decoding
│   └── Encoder.hpp      # Rotary encoder SPI communication (Plus only)
├── avrisp/              # WiFi-based AVR programmer for ATtiny2313
│   ├── main.cpp
│   └── AVRISP.cpp
└── avr/                 # ATtiny2313 firmware for rotary encoders
    ├── main.cpp
    ├── RotaryEncoder.hpp
    └── SPISlave.hpp
```

### Device Variant System

Hardware variants are selected via PlatformIO environment (`DECK` macro):
- `DECK=1` (ORIGINAL_MK2): 5x3 keys, no encoders, no touchscreen
- `DECK=2` (PLUS): 4x2 keys, 4 rotary encoders, touchscreen strip

Each variant includes a TFT config file via `-include`:
- `tft-user-config/StreamDeckOriginalMk2Config.hpp`
- `tft-user-config/StreamDeckPlusConfig.hpp`

### USB HID Protocol

The device implements a custom HID protocol with:
- **Input Reports** (device → host):
  - Report ID 1: Key presses, touch events, encoder turns/presses
- **Output Reports** (host → device):
  - Report ID 0x02: Image data for keys (0x07) or touchscreen (0x0C), reboot commands (0xFF/0xFE)
- **Feature Reports**:
  - Report ID 0x03: Brightness control (0x08), reset command (0x02)
  - Report ID 0x05/0x06: Device version/serial number

Images are transferred as JPEG data in multiple packets and decoded using the JPEGDEC library.

### StreamDeck Plus Specific

The Plus variant uses an **ATtiny2313** microcontroller to handle 4 rotary encoders:
- ATtiny2313 continuously scans encoders and aggregates turn/press state
- Communicates with Pico via SPI (ATtiny as slave, Pico as master)
- Data format: 16-bit value with MSB indicating validity (0x8000 bit)
- Programming: Use `pico_avrisp_wifi` environment to flash ATtiny via WiFi

**Important**: The ATtiny2313 requires avrdude 7.3+ for WiFi programming. PlatformIO's embedded avrdude 6.3 won't work.

## Development Notes

### Display and Touch Calibration

On first boot, the device prompts for touchscreen calibration. Calibration data is stored in LittleFS at `/lcd_calibration.dat`. To force recalibration, call `lcd.calibrate(file_repository, true)` or delete the file.

### Key Ordering

Mk.2 variant uses `KEY_ORDER_REVERSE` - key indices are reversed in hardware layout vs. protocol numbering.

### Testing HID Communication

Use [HID Explorer](https://nondebug.github.io/webhid-explorer/) via WebHID for testing:
- Send `[02, FF]` to output report → enter BOOTSEL mode
- Send `[02, FE]` to output report → reboot device

### Building for StreamDeck Plus

The complete build sequence is:
1. Create and configure `config.ini` with WiFi credentials
2. Build and upload `pico_avrisp_wifi` to Pico W
3. Note the IP address from serial output
4. Update `config.ini` with the Pico's IP address
5. Build and upload `attiny2313` firmware via WiFi
6. Build and upload `pico_deckPlus` firmware

### Common Pitfalls

- **Don't use `wait()` or `delay()` in Core 0's `loop()`** - TinyUSB's `tud_task()` must be called frequently for USB communication
- **Output reports may arrive fragmented** - the firmware accumulates packets until `OUTPUT_REPORT_LEN` bytes are received
- **LCD uses significant current (90mA)** - ensure adequate power supply
- **ATtiny2313 leads may interfere with SDCard slot** - trim leads or remove SD slot
