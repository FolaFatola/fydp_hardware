#include <HardwareSerial.h>

HardwareSerial OBDSerial(2); // UART2

#define RX_PIN 16
#define TX_PIN 17

// buffer to store response data
char rxData[20];
uint8_t rxIndex = 0;

int vehicleSpeed = 0;
int vehicleRPM = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);

  // Init UART2 for OBD-II at 9600 baud
  OBDSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(1000);

  Serial.println("ESP32 OBD-II Reset Command Test");

  Serial.println("Sending ATZ (reset) command...");
  OBDSerial.println("ATZ");
  delay(2000); // Wait for response

  getResponse();
  Serial.print("Response: ");
  Serial.println(rxData);

  // flush
  while (Serial.available()) Serial.read();
}

void loop() {
  // flush
  while (Serial.available()) Serial.read();
  // Get speed
  OBDSerial.println("010D");
  //Get the response from the OBD-II-UART board. We get two responses
  //because the OBD-II-UART echoes the command that is sent.
  //We want the data in the second response.
  getResponse();
  getResponse();

  vehicleSpeed = strtol(&rxData[6],0,16);
  Serial.println("Vehicle speed: " + vehicleSpeed);
  rxIndex = 0;
  delay(100);

  // Get RPM
  OBDSerial.println("010C");
  getResponse();
  getResponse();
  //Convert the string data to an integer
  //NOTE: RPM data is two bytes long, and delivered in 1/4 RPM from the OBD-II-UART
  vehicleRPM = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/4;
  Serial.println("Vehicle RPM: " + vehicleRPM);
  rxIndex = 0;
  delay(100);
}

// Read response from OBD until '\r' or timeout (3 seconds)
void getResponse(void)
{
  char inChar = 0;
  //Keep reading characters until we get a carriage return
  while (inChar != '\r')
  {
    //If a character comes in on the serial port, we need to act on it.
    if (OBDSerial.available() > 0)
    {
      //Start by checking if we've received the end of message character ('\r').
      if (OBDSerial.peek() == '\r')
      {
        //Clear the Serial buffer
        inChar = OBDSerial.read();
        //Put the end of string character on our data string
        rxData[rxIndex] = '\0';
        //Reset the buffer index so that the next character goes back at the beginning of the string.
        rxIndex = 0;
      }
      //If we didn't get the end of message character, just add the new character to the string.
      else
      {
        //Get the new character from the Serial port.
        inChar = OBDSerial.read();
        //Add the new character to the string, and increment the index variable.
        rxData[rxIndex++] = inChar;
      }
    }
  }
}