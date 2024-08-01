#include <SPI.h>
#include "./AVRISP.hpp"

#include <Wire.h>

#if !defined(WIFI_SSID) || !defined(WIFI_PW)
#warning "Please define WIFI_SSID and WIFI_PW in your config"
#define WIFI_SSID "your-ssid"
#define WIFI_PW "your-password"
#endif

const uint16_t port = 328;
const uint8_t reset_pin = 17;

AVRISP avrprog(port, reset_pin, 1000000UL / 6);

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("");
  Serial.println("Arduino AVR-ISP over TCP");
  avrprog.setReset(false); // let the AVR run

  WiFi.mode(WIFI_STA);
  while (1)
  {
    WiFi.begin(WIFI_SSID, WIFI_PW);
    Serial.println("Connecting to WiFi");
    for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++)
    {
      delay(1000);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      break;
    }
    WiFi.end();
    delay(5000);
  }

  IPAddress local_ip = WiFi.localIP();
  Serial.print("IP address: ");
  Serial.println(local_ip);
  Serial.println("Use your avrdude:");
  Serial.print("avrdude -c arduino -p <device> -P net:");
  Serial.print(local_ip);
  Serial.print(":");
  Serial.print(port);
  Serial.println(" -t # or -U ...");

  // listen for avrdudes
  avrprog.begin();
}

#if defined(DEBUG_SPI)
bool isInited = false;
uint32_t latestMillis = 0;
#endif

void loop()
{
  static AVRISPState_t last_state = AVRISP_STATE_INIT;
  AVRISPState_t new_state = avrprog.update();
  if (last_state != new_state)
  {
    switch (new_state)
    {
    case AVRISP_STATE_IDLE:
    {
      Serial.printf("[AVRISP] now idle\r\n");
      // Use the SPI bus for other purposes
#if defined(DEBUG_SPI)
      if (!isInited)
      {
        SPI.begin();
        SPI.beginTransaction(SPISettings(8000000UL / 8 / 16, MSBFIRST, SPI_MODE0));
        isInited = true;
      }
#endif
      break;
    }
    case AVRISP_STATE_PENDING:
    {
      Serial.printf("[AVRISP] connection pending\r\n");
      // Clean up your other purposes and prepare for programming mode
#if defined(DEBUG_SPI)
      SPI.end();
      isInited = false;
#endif
      break;
    }
    case AVRISP_STATE_ACTIVE:
    {
      Serial.printf("[AVRISP] programming mode\r\n");
      // Stand by for completion
      break;
    }
    }
    last_state = new_state;
  }

#if defined(DEBUG_SPI)
  if (isInited && millis() - latestMillis > 10)
  {
    latestMillis = millis();
    uint32_t start = micros();
    SPI.transfer(0);
    uint8_t lowerRxbuf = SPI.transfer(0);
    SPI.transfer(1);
    uint8_t upperRxbuf = SPI.transfer(1);
    uint16_t rxbuf = (upperRxbuf << 8) | lowerRxbuf;
    if (rxbuf > 0x8000)
    {
      Serial.printf("Received(%03d[us]): ", micros() - start);
      Serial.println(rxbuf, BIN);
    }
    else if (rxbuf != 0x8000)
    {
      Serial.printf("Received invalid data(%03d[us]): ", micros() - start);
      Serial.println(rxbuf, BIN);
    }
  }
#endif

  // Serve the client
  if (last_state != AVRISP_STATE_IDLE)
  {
    avrprog.serve();
  }
}