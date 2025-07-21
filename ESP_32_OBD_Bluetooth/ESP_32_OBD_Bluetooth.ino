#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>

constexpr int scan_time = 10;

bool found_device = false;
volatile bool received_response_data = false;
String device_name = "VEEPEAK";
BLEAdvertisedDevice my_device;

String serviceUUID = "0000fff0-0000-1000-8000-00805f9b34fb";
String readCharacteristicUUID = "0000fff1-0000-1000-8000-00805f9b34fb";
String writeCharacteristicUUID = "0000fff2-0000-1000-8000-00805f9b34fb";

//characteristics to read from and write to obd2 scanner
BLERemoteCharacteristic* pMyRemoteReadCharacteristic = nullptr;
BLERemoteCharacteristic* pMyRemoteWriteCharacteristic = nullptr;


//OBD reading data set...
String rx_data = "";

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) { //Scan for surroundin bluetooth devices.
    if (advertisedDevice.haveName()) {
      if (advertisedDevice.getName() == device_name) {
        Serial.println("We have connected with VeePEAK");
        my_device = advertisedDevice;
        found_device = true;
        advertisedDevice.getScan()->stop(); //We have found the device, so stop the scan.
      }

    }
  }
};

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  
  // Serial.print("Notification: ");
  for (int i = 0; i < length; i++) {
    // Serial.print((char)pData[i]);
    rx_data += (char)pData[i];
  }
  received_response_data = true;
}

bool initOBD() {
  String init_command[5] = {"ATZ\r", "ATE0\r", "ATL0\r", "ATS0\r", "ATSP0\r"};
  for (int command_idx = 0; command_idx < 5; ++command_idx) {
    pMyRemoteWriteCharacteristic->writeValue(init_command[command_idx]);
    delay(1000);

    if (!received_response_data) {
      return false;
    }
    received_response_data = false;
  }
  return true;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting BLE work!");

  // Initialize BLE with device name
  BLEDevice::init("Veepeak_Connect");

  //scan for devices
  BLEScan* pMyScan = BLEDevice::getScan();
  Serial.println("Starting to scan for devices....");
  pMyScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); //register callback in order to scan for Bluetooth devices.
  pMyScan->start(scan_time); //returns 

  Serial.println("The name of the device is " + my_device.getName());

  unsigned long start_time = millis();
  unsigned long end_time;
  while (!found_device || end_time - start_time < 2000) {
    end_time = millis();
  }

  //Create client. ESP32 is the client
  BLEClient* pMyClient = BLEDevice::createClient();
  if (!pMyClient->connect(my_device.getAddress())) { //connect to device
    Serial.println("Failed to connect");
  } else {  //If we can connect to the scanner...
    BLERemoteService* pMyRemoteService = pMyClient->getService(my_device.getServiceUUID());
    pMyRemoteReadCharacteristic = pMyRemoteService->getCharacteristic(readCharacteristicUUID);
    pMyRemoteWriteCharacteristic = pMyRemoteService->getCharacteristic(writeCharacteristicUUID);

    if (pMyRemoteReadCharacteristic == nullptr || pMyRemoteWriteCharacteristic == nullptr) {
      Serial.println("Failed to find characteristics.");
      return;
    }

    pMyRemoteReadCharacteristic->registerForNotify(notifyCallback);

    bool init_success = initOBD();
    Serial.flush();
  }
}


String parse(String command, String response) {
  int skipPos = response.indexOf(command) + command.length();
  String spaceless_string;

  if (skipPos == -1) {
      return "Failure";
  } else {
    String obd_information = response.substring(skipPos);
    for (char c : obd_information) {
        if (c != ' ' && c != '\r' && c != '>') {
            spaceless_string += c;
        }
    }
  }
  return spaceless_string;
}


int fromChartoInt(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  } else {
    return -1;
  }
}

String reverseString(String str) {
  int len = str.length();
  String result = "";

  for (int i = len - 1; i >= 0; i--) {
    result += str[i];
  }

  return result;
}



void loop() {
  unsigned long start = millis();
  unsigned long end = 0;
  int speed_one = 0;
  int speed_two = 0;
  float speed_one_mps = 0;
  float speed_two_mps = 0;
  float acceleration = 0;

  rx_data.clear();
  //request speed
  pMyRemoteWriteCharacteristic->writeValue("010D\r");
  delay(200);
  if (received_response_data) {
    String spaceless_string = parse("41 0D", rx_data);
    rx_data.clear();

    speed_one = strtol(spaceless_string.c_str(), NULL, 16);
    
    Serial.print("Speed: ");
    Serial.print((float)speed_one / 3.6);

    received_response_data = false; //reset flag.
  } else {
    Serial.print("EMPTY");
  }

  rx_data.clear();
  Serial.print(", ");

  //second speed request for acceleration.
  pMyRemoteWriteCharacteristic->writeValue("010D\r");
  delay(200);
  if (received_response_data) {
    String spaceless_string = parse("41 0D", rx_data);
    rx_data.clear();

    speed_two = strtol(spaceless_string.c_str(), NULL, 16);
    end = millis();

    acceleration = (((float)speed_two/3.6) - ((float)speed_one / 3.6)) / ((float)(end - start) / 1000.0);
    Serial.print(" Acceleration: ");
    Serial.print(acceleration);

    received_response_data = false; //reset flag.
  } else {
    Serial.print("Acceleration: EMPTY");
  }

  Serial.print(", ");
  rx_data.clear();
  //request engine RPM
  pMyRemoteWriteCharacteristic->writeValue("010C\r");
  delay(200);
  if (received_response_data) {
    String spaceless_string = parse("41 0C", rx_data);

    rx_data.clear();

    int rpm = 0;
    if (spaceless_string.length() < 4) {
        Serial.print("RPM: EMPTY");
  
    } else {
      int A = fromChartoInt(spaceless_string[0]) * 16 + fromChartoInt(spaceless_string[1]);
      int B = fromChartoInt(spaceless_string[2]) * 16 + fromChartoInt(spaceless_string[3]);
      rpm = ((A * 256) + B) / 4;
      Serial.print(" RPM: ");
      Serial.print(rpm);
    }
    

    received_response_data = false; //reset flag.
  } else {
    Serial.print("RPM: EMPTY");
  }

  Serial.print(", ");
  rx_data.clear();
  //request engine load
  pMyRemoteWriteCharacteristic->writeValue("0104\r");
  delay(200);
  if (received_response_data) {
    String spaceless_string = parse("41 04", rx_data);
    rx_data.clear();

    float engine_load = (100.0 / 255 ) * (float)strtol(spaceless_string.c_str(), NULL, 16);
    Serial.print(" Engine Load: ");
    Serial.print(engine_load);

    received_response_data = false; //reset flag.
  } else {
    Serial.print("Engine load: EMPTY");
  }

  Serial.println("\n");
}
