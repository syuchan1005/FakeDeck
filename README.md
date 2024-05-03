# Fakedeck
Using raspberry pi pico as a stream deck

![](./assets/example.jpg)


## Avaiable devices - Can be modified from [src/deck_config.hpp](src/deck_config.hpp)
- Stream deck v2
- WIP: Stream deck Plus

## How to create
### Parts
- Raspberry Pi Pico
- ILI9488 TFT with Touch: [AliExpress](https://www.aliexpress.com/item/32985467436.html)
> [!IMPORTANT]
> Make sure to select "touch screen"
- Solder, wire, etc.
- 3D printers for creating a case - [Top](./assets/Case%20-%20Top.stl), [Bottom](./assets/Case%20-%20Bottom.stl)
- M3 x 6mm screw *4

### Wire
![wire](./assets/wire.jpg)
![wire example](./assets/wire_example.jpg)

> [!NOTE]
> LCD uses 90 mA. 
> In the example, the back of the LCD is shaved and soldered to hold the pico in place.

### Build
1. Setup [PlatformIO IDE](https://platformio.org/platformio-ide)
2. Clone and open this project
3. Build .uf2 file!

### Tips
- [HID Explorer](https://nondebug.github.io/webhid-explorer/) is an easy HID testing tool using WebHID
- Reboot device and enter bootsel mode: Send `[02, FF]` to the output report
- Reboot device: Send `[02, FE]` to the output report

## TODO
- Make USB and LCD buffer
- Support touchscreen and dial report - for Plus
- Support infobar and touch key - for Neo
