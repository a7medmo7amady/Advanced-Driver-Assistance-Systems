/*
 * Bluetooth (HC-05) Test Sketch
 * Tests HC-05 module connection
 * 
 * IMPORTANT: Use voltage divider on HC-05 RX pin!
 * Arduino A3 --[1kΩ]--+--[2kΩ]-- GND
 *                     |
 *                 HC-05 RX
 * 
 * To test:
 * 1. Upload this sketch
 * 2. Pair your phone/laptop with HC-05 (PIN: 1234 or 0000)
 * 3. Connect using a Bluetooth Serial app (e.g., "Serial Bluetooth Terminal")
 * 4. Type messages and see them on Serial Monitor
 * 5. Type in Serial Monitor and see them on your phone
 */

#include <SoftwareSerial.h>

const int BT_RX = A2;const int BT_TX = A3; 

SoftwareSerial bluetooth(BT_RX, BT_TX);

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);  
  
  Serial.println("Bluetooth Test");
  
  bluetooth.println("Hello from Arduino ADAS!");
  bluetooth.println("Bluetooth connection successful!");
}

void loop() {
  if (bluetooth.available()) {
    char c = bluetooth.read();
    Serial.write(c);
  }
  
  if (Serial.available()) {
    char c = Serial.read();
    bluetooth.write(c);
  }
}

