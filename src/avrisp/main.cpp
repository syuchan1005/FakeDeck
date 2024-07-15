#include <SPI.h>
#include "./AVRISP.hpp"

#include <Wire.h>

#if !defined(WIFI_SSID) || !defined(WIFI_PW)
  #warning "Please define WIFI_SSID and WIFI_PW in your config"
  #define WIFI_SSID "your-ssid"
  #define WIFI_PW "your-password"
#endif

const uint16_t port = 328;
const uint8_t reset_pin = 15;

AVRISP avrprog(port, reset_pin, 1000000UL / 6);

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("");
  Serial.println("Arduino AVR-ISP over TCP");
  avrprog.setReset(false); // let the AVR run

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PW);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
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

bool isInited = false;
uint32_t latestMillis = 0;

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
      if (!isInited)
      {
        SPI.begin();
        SPI.beginTransaction(SPISettings(1000000UL/6, MSBFIRST, SPI_MODE0));
        isInited = true;
      }
      break;
    }
    case AVRISP_STATE_PENDING:
    {
      Serial.printf("[AVRISP] connection pending\r\n");
      // Clean up your other purposes and prepare for programming mode
      SPI.end();
      isInited = false;
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

  if (isInited && millis() - latestMillis > 1000)
  {
    latestMillis = millis();
    uint16_t rxbuf = SPI.transfer16(0);
    Serial.printf("Received: %02X\r\n", rxbuf);
  }

  // Serve the client
  if (last_state != AVRISP_STATE_IDLE)
  {
    avrprog.serve();
  }
}