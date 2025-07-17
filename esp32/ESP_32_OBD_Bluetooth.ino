#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>

// BLE variables
BLEScan *pBLEScan;
BLEClient *pClient;
BLERemoteService *pService;
BLERemoteCharacteristic *pTxCharacteristic;
BLERemoteCharacteristic *pRxCharacteristic;

// OBD variables
String veepeak_address = "8C:DE:52:DE:A2:6F";
bool dev_connected = false;
String rxData = "";

// For acceleration calculation
float prev_speed = 0;
unsigned long prev_time = 0;

// BLE UUIDs for Veepeak OBD
#define SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define TX_UUID "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define RX_UUID "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting OBD BLE...");

  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);

  // Scan for device
  Serial.println("Scanning for Veepeak OBD...");
  BLEScanResults results = pBLEScan->start(5);

  bool found = false;
  for (int i = 0; i < results.getCount(); i++)
  {
    BLEAdvertisedDevice device = results.getDevice(i);
    if (device.getAddress().toString() == veepeak_address)
    {
      Serial.println("Found Veepeak OBD!");
      found = true;
      break;
    }
  }

  if (!found)
  {
    Serial.println("Veepeak OBD not found!");
    return;
  }

  // Connect to device
  pClient = BLEDevice::createClient();
  BLEAddress addr(veepeak_address);

  if (pClient->connect(addr))
  {
    Serial.println("Connected to Veepeak OBD!");

    // Get service and characteristics
    pService = pClient->getService(SERVICE_UUID);
    if (pService != nullptr)
    {
      pTxCharacteristic = pService->getCharacteristic(TX_UUID);
      pRxCharacteristic = pService->getCharacteristic(RX_UUID);

      if (pTxCharacteristic != nullptr && pRxCharacteristic != nullptr)
      {
        // Subscribe to notifications
        pRxCharacteristic->registerForNotify([](BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
                                             {
          for(int i = 0; i < length; i++) {
            char c = (char)pData[i];
            if(c == '>') {
              Serial.println("Received: " + rxData);
              rxData = "";
            } else if(c != '\r' && c != '\n') {
              rxData += c;
            }
          } });

        dev_connected = true;
        initializeOBD();
        prev_time = millis();
      }
    }
  }
}

void sendOBD(String cmd)
{
  Serial.print("Sending: ");
  Serial.println(cmd);
  if (pTxCharacteristic && dev_connected)
  {
    String command = cmd + "\r";
    pTxCharacteristic->writeValue((uint8_t *)command.c_str(), command.length());
    delay(100);
  }
}

void readOBD(String &responseBytes)
{
  unsigned long start = millis();
  while (millis() - start < 2000)
  { // 2-second timeout
    if (rxData.length() > 0)
    {
      responseBytes = rxData;
      rxData = "";
      Serial.println(responseBytes);
      return;
    }
    delay(10);
  }
  Serial.println("Timeout waiting for OBD response");
}

int getSpeed()
{
  String response;
  getOBDdata("010D");
  readOBD(response);

  if (response.length() < 6)
    return -1; // Response too short

  // Response should now look like: "410D3C"
  String speedHex = response.substring(response.length() - 2); // Last 2 chars
  return strtol(speedHex.c_str(), NULL, 16);
}

int getRPM()
{
  String response;
  getOBDdata("010C");
  readOBD(response);

  if (response.length() < 8)
    return -1; // Response too short

  // Response should look like: "410C1A0B" (A=1A, B=0B)
  String A_str = response.substring(4, 6);
  String B_str = response.substring(6, 8);
  int A = strtol(A_str.c_str(), NULL, 16);
  int B = strtol(B_str.c_str(), NULL, 16);
  return ((A * 256) + B) / 4;
}

float calculateAcceleration(float current_speed)
{
  unsigned long current_time = millis();
  float time_diff = (current_time - prev_time) / 1000.0;

  if (time_diff > 0 && prev_time > 0)
  {
    float speed_diff = (current_speed - prev_speed) * 0.277778; // km/h to m/s
    float acceleration = speed_diff / time_diff;

    prev_speed = current_speed;
    prev_time = current_time;

    return acceleration;
  }

  prev_speed = current_speed;
  prev_time = current_time;
  return 0;
}

void initializeOBD()
{
  sendOBD("ATZ"); // reset
  delay(1000);
  sendOBD("ATE0");
  delay(200); // Echo Off
  sendOBD("ATL0");
  delay(200); // Linefeeds Off
  sendOBD("ATS0");
  delay(200); // Spaces Off
  sendOBD("ATH0");
  delay(200); // Headers Off
  sendOBD("ATPS0");
  delay(1000);
}

void getOBDdata(String pid)
{
  sendOBD(pid); // e.g., "010D" = Speed PID
}

void loop()
{
  if (dev_connected)
  { // send and receive data.
    Serial.println("Device connected");

    int speed_data = getSpeed();
    int rpm_data = getRPM();

    if (speed_data != -1 && rpm_data != -1)
    {
      float acceleration = calculateAcceleration(speed_data);

      Serial.print("Speed: ");
      Serial.print(speed_data);
      Serial.println(" km/h");
      Serial.print("RPM: ");
      Serial.print(rpm_data);
      Serial.println(" rpm");
      Serial.print("Acceleration: ");
      Serial.print(acceleration, 2);
      Serial.println(" m/s²");
    }
    else
    {
      Serial.println("Invalid OBD response");
    }

    delay(2000);
  }
  else
  {
    Serial.println("Device not connected");
    delay(5000);
  }
}
