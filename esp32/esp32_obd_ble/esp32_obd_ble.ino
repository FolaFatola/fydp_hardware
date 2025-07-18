#include "ELMduino.h"
#include <BluetoothSerial.h>


#define DEBUG_PORT Serial
#define ELM_PORT   BLESerial

ELM327 myELM327;

uint32_t rpm = 0;


void setup()
{
  DEBUG_PORT.begin(115200);
  // Change name based on your OBD scanner
  ELM_PORT.begin("VEEPEAK");
  if (!myELM.begin(SerialBT, true, 2000)) {
    Serial.println("Failed to connect");
    while (1);
  }
  Serial.println("Connected");
}

void loop()
{
  float tempRPM = myELM327.rpm();

  if (myELM327.nb_rx_state == ELM_SUCCESS)
  {
    rpm = (uint32_t)tempRPM;
    Serial.print("RPM: "); Serial.println(rpm);
  }
  else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    myELM327.printError();

}