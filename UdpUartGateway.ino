#define ESP8266 1
#define ESP32 2
#define BOARD ESP32

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
#define RXD 16
#define TXD 17

#include "wifi.h"

AsyncUDP udp;

IPAddress broadcastIp(255,255,255,255);

char charmac[18];
uint8_t mac[6];

void setup() {
  Serial.begin(115200);
  Serial.println();

  UartSerial.begin(921600, SERIAL_8N1, RXD, TXD);

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf(" connected with %s\n", WiFi.localIP().toString());

  WiFi.macAddress(mac);
  sprintf(charmac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if(udp.listen(rx_port)) {
    Serial.println("UDP listening");
    udp.onPacket([](AsyncUDPPacket packet) {
      if (packet.localPort() != rx_port) { // Not sure this is useful
        return;
      }
      uint8_t* data = packet.data();
      int length = packet.length();
      size_t c = UartSerial.write(data, length);
      Serial.printf("Forwarded udp->uart [%d]->[%d] {", length, c);
      for (int j = 0; j < length; j++) {
        Serial.printf("%02X", data[j]);
      }
      Serial.println("}");
    });
  } else {
    Serial.print("UDP not connected");
  }
}

void loop() {
  delay(1); //WARN This may cause problems, but I didn't want to emit a bunch of short udp packets

  int i = 0;
  uint8_t buffer[1024];
  while (UartSerial.available() > 0) {
    buffer[i++] = UartSerial.read();
  }
  if (i > 0) {
    size_t c = udp.broadcastTo(buffer, i, tx_port);
    Serial.printf("Forwarded uart->udp [%d]->[%d] {", i, c);
    for (int j = 0; j < i; j++) {
      Serial.printf("%02X", buffer[j]);
    }
    Serial.println("}");
  }
}