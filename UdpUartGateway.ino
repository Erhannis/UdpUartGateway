#define ESP8266 1
#define ESP32 2
#define BOARD ESP8266
#define EMIT_RUNNING 0

//PERIODIC If ESP32, rename AsyncUDP.cpp to AsyncUDP.cpp0, else rename it back

#if BOARD == ESP8266
  #include <ESP8266WiFi.h>
  #include "ESPAsyncUDP.h" // https://github.com/me-no-dev/ESPAsyncUDP
#elif BOARD == ESP32
  #include <WiFi.h>
  #include <AsyncUDP.h>
#endif

#include <HardwareSerial.h>
HardwareSerial UartSerial(2);
#define RXD 16 // RX2
#define TXD 17 // TX2

#define USB 1
#if USB
#define LOGGING UartSerial // Pins
#define COMMS Serial // Usb
#else
#define LOGGING Serial // Usb
#define COMMS UartSerial // Pins
#endif

#include "wifi.h"

AsyncUDP udp;

IPAddress broadcastIp(255,255,255,255);

char charmac[18];
uint8_t mac[6];

void setup() {
  Serial.begin(115200);
  Serial.println();

#if BOARD == ESP8266
  UartSerial.begin(115200, SERIAL_8N1, SERIAL_FULL, RXD); //CHECK I'm not sure if this works, uses a hardcoded TXD, or what
#elif BOARD == ESP32
  UartSerial.begin(115200, SERIAL_8N1, RXD, TXD);
#endif


  LOGGING.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    LOGGING.print(".");
  }
  LOGGING.printf(" connected with %s\n", WiFi.localIP().toString());

  WiFi.macAddress(mac);
  sprintf(charmac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if(udp.listen(rx_port)) {
    LOGGING.println("UDP listening");
    udp.onPacket([](AsyncUDPPacket packet) {
      if (packet.localPort() != rx_port) { // Not sure this is useful
        return;
      }
      uint8_t* data = packet.data();
      int length = packet.length();
      size_t c = COMMS.write(data, length);
      #if BOARD == ESP8266
        COMMS.flush();
      #elif BOARD == ESP32
        COMMS.flush(true);
      #endif
      LOGGING.printf("Forwarded udp->uart [%d]->[%d] {", length, c);
      for (int j = 0; j < length; j++) {
        LOGGING.printf("%02X", data[j]);
      }
      LOGGING.println("}");
    });
  } else {
    LOGGING.print("UDP not connected");
  }
  const char* msg = "UDP/UART start\n";
  size_t c = udp.broadcastTo((uint8_t*)msg, strlen(msg), tx_port);
  LOGGING.printf("Forwarded uart->udp [%d]->[%d] @%d", strlen(msg), c, tx_port);
}

int lastPing = 0;

void loop() {
  delay(1); //WARN This may cause problems, but I didn't want to emit a bunch of short udp packets

  int i = 0;
  uint8_t buffer[1024];
  while (COMMS.available() > 0) {
    buffer[i++] = COMMS.read();
  }
  if (i > 0) {
    size_t c = udp.broadcastTo(buffer, i, tx_port);
    LOGGING.printf("Forwarded uart->udp [%d]->[%d] {", i, c);
    for (int j = 0; j < i; j++) {
      LOGGING.printf("%02X", buffer[j]);
    }
    LOGGING.println("}");
  }

  if (EMIT_RUNNING && millis() - lastPing > 1000) {
    const char* msg = "UDP/UART running\n";
    size_t c = udp.broadcastTo((uint8_t*)msg, strlen(msg), tx_port);
    lastPing = millis();
  }
}