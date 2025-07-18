#include <Arduino.h>
#include <BluetoothSerial.h>
#include "ELMduino.h"

BluetoothSerial SerialBT;
ELM327 myELM;

uint32_t rpm = 0;


void setup()
{
  Serial.begin(115200);
  SerialBT.begin("VEEPEAK");
  
  if (!myELM.begin(SerialBT, true, 2000)) {
    Serial.println("Failed to connect");
    while (1);
  }
  Serial.println("Connected");
}

void loop()
{
    if (myELM.resetDTC())
    {
      Serial.println("DTC reset successful.");
    }
    else
    {
      Serial.println("DTC reset failed.");
    }
}