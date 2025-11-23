#include <Arduino.h>

// ESP32 DevKit v1 – mostek USB<->UART2 (do CORE2)
HardwareSerial core2Serial(2);  // UART2: RX=16, TX=17

void setup() {
  // USB do PC
  Serial.begin(115200);
  delay(500);

  // UART do CORE2: RX=16, TX=17
  core2Serial.begin(115200, SERIAL_8N1, 16, 17);

  Serial.println("ESP32 bridge: PC -> CORE2 (hExt.serial)");
  Serial.println("Wysylaj znaki: A/a, B/b, C/c aby sterowac hLED1/2/3.");
}

void loop() {
  // PC -> CORE2
  while (Serial.available() > 0) {
    int c = Serial.read();
    core2Serial.write(c);
  }

  // CORE2 -> PC (gdybys chcial cos wypisywac z CORE2)
  while (core2Serial.available() > 0) {
    int c = core2Serial.read();
    Serial.write(c);
  }
}
