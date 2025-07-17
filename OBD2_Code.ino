#include <HardwareSerial.h>

HardwareSerial OBDSerial(2); // UART2

#define RX_PIN 16
#define TX_PIN 17

char rxData[100];
uint8_t rxIndex = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("ESP32 OBD-II Reset Command Test");

  // Init UART2 for OBD-II at 9600 baud
  OBDSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  delay(1000);

  Serial.println("Sending ATZ (reset) command...");
  OBDSerial.println("ATZ");
  delay(2000); // Wait for response
  OBDSerial.println("ATEO");
    delay(2000); // Wait for response
  OBDSerial.println("ATL0");
    delay(2000); // Wait for response
  OBDSerial.println("ATS0");
    delay(2000); // Wait for response
  OBDSerial.println("ATH0");
    delay(2000); // Wait for response
  OBDSerial.println("ATSP0");

  getResponse();
  Serial.print("Response: ");
  Serial.println(rxData);
}

int fromCharToNum(char character) {
  if (character >= '0' && character <= '9') {
    return character - '0';
  } else if (character >= 'A' && character <= 'F') {
    return character - 'A' + 10;
  } else if (character >= 'a' && character <= 'f') {
    return character - 'a' + 10;
  } else {
    return -1;
  }
}

void loop() {
  // Nothing here
  OBDSerial.println("010D");
  delay(2000);
  getResponse();
  Serial.println(rxData);
}

// Read response from OBD until '\r' or timeout (3 seconds)
void getResponse() {
  rxIndex = 0;
  unsigned long start = millis();

  while ((millis() - start) < 3000) {
    if (OBDSerial.available()) {
      char c = OBDSerial.read();
      if (c == '\r') {
        rxData[rxIndex] = '\0';
        return;
      } else if (rxIndex < sizeof(rxData) - 1) {
        rxData[rxIndex++] = c;
      }
    }
  }
  rxData[rxIndex] = '\0'; // Null-terminate on timeout
}
