ESP32 (or ESP8266) program, copies data from UDP packets to UART2, and vice versa.  Copy wifi.h.default to wifi.h, then fill in the wifi ssid and password.  Uart is configured for 921600 b/s.  If you want to run it on ESP8266, see the top of the .ino for instructions.

MIT license, except for AsyncUDP and ESPAsyncUDP which I'm pretty sure I copied from somewhere else.

-Erhannis