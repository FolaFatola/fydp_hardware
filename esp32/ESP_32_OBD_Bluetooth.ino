#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAddress.h>
#include <BLE2902.h>

// Veepeak BLE OBD-II UART Service/Characteristic UUIDs
#define UART_SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define UART_CHAR_TX_UUID "6e400002-b5a3-f393-e0a9-e50e24dcca9e" // Write
#define UART_CHAR_RX_UUID "6e400003-b5a3-f393-e0a9-e50e24dcca9e" // Notify

String veepeak_obd_address = "8C:DE:52:DE:A2:6F"; // Insert your BLE address here
BLEAddress *pServerAddress = nullptr;
BLEClient *pClient = nullptr;
BLERemoteCharacteristic *pTxCharacteristic = nullptr;
BLERemoteCharacteristic *pRxCharacteristic = nullptr;
bool dev_connected = false;
String rxBuffer = "";

// Callback for receiving notifications from RX characteristic
class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    Serial.println("Connected to BLE server");
  }
  void onDisconnect(BLEClient *pclient)
  {
    Serial.println("Disconnected from BLE server");
    dev_connected = false;
  }
};

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  for (size_t i = 0; i < length; i++)
  {
    char c = (char)pData[i];
    if (c == '>')
    {
      // End of OBD response
      Serial.println(rxBuffer);
      rxBuffer = "";
    }
    else if (c != '\r' && c != '\n')
    {
      rxBuffer += c;
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE Client for Veepeak OBD-II");
  BLEDevice::init("");
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  BLEScanResults foundDevices = pBLEScan->start(5);
  bool found = false;
  for (int i = 0; i < foundDevices.getCount(); i++)
  {
    BLEAdvertisedDevice d = foundDevices.getDevice(i);
    if (d.getAddress().toString() == veepeak_obd_address)
    {
      found = true;
      pServerAddress = new BLEAddress(d.getAddress());
      break;
    }
  }
  if (!found)
  {
    Serial.println("Veepeak OBD BLE device not found!");
    return;
  }
  Serial.println("Found Veepeak OBD BLE device, connecting...");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  if (!pClient->connect(*pServerAddress))
  {
    Serial.println("Failed to connect to server");
    return;
  }
  BLERemoteService *pService = pClient->getService(UART_SERVICE_UUID);
  if (pService == nullptr)
  {
    Serial.println("UART service not found!");
    pClient->disconnect();
    return;
  }
  pTxCharacteristic = pService->getCharacteristic(UART_CHAR_TX_UUID);
  pRxCharacteristic = pService->getCharacteristic(UART_CHAR_RX_UUID);
  if (pTxCharacteristic == nullptr || pRxCharacteristic == nullptr)
  {
    Serial.println("UART characteristics not found!");
    pClient->disconnect();
    return;
  }
  if (pRxCharacteristic->canNotify())
  {
    pRxCharacteristic->registerForNotify(notifyCallback);
  }
  dev_connected = true;
  Serial.println("Connected to Veepeak OBD BLE!");
  initializeOBD();
}

void sendOBD(String cmd)
{
  Serial.print("Sending: ");
  Serial.println(cmd);
  if (pTxCharacteristic)
  {
    String toSend = cmd + "\r";
    pTxCharacteristic->writeValue((uint8_t *)toSend.c_str(), toSend.length());
  }
}

// Wait for response in rxBuffer (with timeout)
bool readOBD(String &responseBytes, uint32_t timeout = 2000)
{
  responseBytes = "";
  unsigned long start = millis();
  while (millis() - start < timeout)
  {
    if (rxBuffer.length() > 0)
    {
      responseBytes = rxBuffer;
      rxBuffer = "";
      return true;
    }
    delay(10);
  }
  Serial.println("Timeout waiting for OBD response");
  return false;
}

int getSpeed()
{
  String response;
  sendOBD("010D");
  if (!readOBD(response))
    return -1;
  if (response.length() < 6)
    return -1;
  String speedHex = response.substring(response.length() - 2);
  return strtol(speedHex.c_str(), NULL, 16);
}

void initializeOBD()
{
  sendOBD("ATZ");
  delay(1000);
  sendOBD("ATE0");
  delay(200);
  sendOBD("ATL0");
  delay(200);
  sendOBD("ATS0");
  delay(200);
  sendOBD("ATH0");
  delay(200);
  sendOBD("ATPS0");
  delay(1000);
}

void loop()
{
  if (dev_connected)
  {
    Serial.println("Device connected");
    int speed_data = getSpeed();
    if (speed_data == -1)
    {
      Serial.println("Invalid OBD response");
    }
    else
    {
      Serial.print("Speed: ");
      Serial.println(speed_data);
    }
    delay(2000); // Poll every 2 seconds
  }
  else
  {
    Serial.println("Device not connected");
    delay(5000);
  }
}
