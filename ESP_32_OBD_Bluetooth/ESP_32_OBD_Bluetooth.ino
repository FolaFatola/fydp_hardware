#include <BLEDevice.h>
// #include <BLEUtils.h>
#include <BLEServer.h>
// #include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include <HTTPClient.h>
#include "time.h"
#include <WebSocketsClient.h>
#include "obd_parser_logic.hpp"
#include "miscellaneous.hpp"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "gps.hpp"
#include <Wire.h>

WebSocketsClient webSocket;
Adafruit_MPU6050 mpu;
WebPacket web_packet;
WebPacket web_packet_local;
String lineBuf = "";
String message = "";


const char* ssid = "hotspotmaya";
const char* password = "Sarah123";
static const uint16_t GPS2IP_PORT = 11123;

WiFiClient gpsClient;

String serverUrl = "http://driving-buddy-public-44aec6b920a3.herokuapp.com:80/esp32";


bool setup_success = false;
volatile bool received_response_data = false;
volatile bool received_lane_deviation = false;
String lane_deviation = "";
String* volatile lane_deviation_info = &lane_deviation;

String device_name = "VEEPEAK";

const char* serviceUUID = "0000fff0-0000-1000-8000-00805f9b34fb";
const char* serverCharacteristicUUID = "0000fff2-0200-1010-8009-00803f9b34fb";
const char* readCharacteristicUUID = "0000fff1-0000-1000-8000-00805f9b34fb";
const char* writeCharacteristicUUID = "0000fff2-0000-1000-8000-00805f9b34fb";

//characteristics to read from and write to obd2 scanner
BLERemoteCharacteristic* pMyRemoteReadCharacteristic = nullptr;
BLERemoteCharacteristic* pMyRemoteWriteCharacteristic = nullptr;
BLEClient* pMyClient = nullptr;

constexpr uint8_t delayms = 200;

//OBD reading data set...=
String rx_data = "";
String rx_data_2 = "";

constexpr int success_status_LED = 13;
constexpr int lane_deviation_BLE_LED = 26;
constexpr int send_to_data_base_LED = 32;


SemaphoreHandle_t xDataMutex;

// ===== CONNECT TO WIFI =====
bool connectToWifi(wifi_mode_t mode,
                   const char* ssid,
                   const char* password,
                   int num_tries,
                   int timeOutMs) {
    WiFi.mode(mode);

    for (int trial = 0; trial < num_tries; trial++) {
        WiFi.begin(ssid, password);
        int status = WiFi.waitForConnectResult(timeOutMs);

        if (status == WL_CONNECTED) {
            Serial.println("Connection Successful");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            return true;
        }

        Serial.println("Retrying connection...");
    }

    Serial.println("WiFi connection failed");
    return false;
}

// ===== DEFINE WEBSOCKET FUNCTION =====
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Disconnected!");
            break;

        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to: %s\n", payload);
            webSocket.sendTXT("6880f8b67b7507ab0375e09e");
            break;

        case WStype_TEXT:
            Serial.printf("[WSc] Received: %s\n", payload);
            break;
    }
}

//sending data to server
bool sendHTTPData(String message, String serverURL) {
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "text/plain");
  
  int httpResponseCode = http.POST(message);
  // Serial.print("The server code is: ");
  // Serial.println(httpResponseCode); 
  if (httpResponseCode != 200) {
    return false;
  } 

  http.end();
  return true;
}

bool parseRMC_latlon(const String& line, double& lat, double& lon);

bool parseRMC_latlon(const String& line, double& lat, double& lon) {
  if (!(line.startsWith("$GPRMC,") || line.startsWith("$GNRMC,"))) return false;

  int p[7];
  int idx = 0;

  for (int i = 0; i < line.length() && idx < 7; i++) {
    if (line[i] == ',') {
      p[idx++] = i;
    }
  }
  if (idx < 7) return false;
  
  // status field
  String status = line.substring(p[1] + 1, p[2]);
  if (status != "A") return false;  // only valid fixes

  String latDM  = line.substring(p[2] + 1, p[3]);
  String latH   = line.substring(p[3] + 1, p[4]);
  String lonDM  = line.substring(p[4] + 1, p[5]);
  String lonH   = line.substring(p[5] + 1, p[6]);
  
  if (latDM.length() == 0 || lonDM.length() == 0) return false;
  if (latH.length() == 0 || lonH.length() == 0) return false;

  // convert DDMM.MMMMM → decimal
  double latRaw = latDM.toFloat();
  int latDeg = (int)(latRaw / 100.0);
  double latMin = latRaw - latDeg * 100.0;
  lat = latDeg + latMin / 60.0;
  if (latH[0] == 'S') lat = -lat;

  double lonRaw = lonDM.toFloat();
  int lonDeg = (int)(lonRaw / 100.0);
  double lonMin = lonRaw - lonDeg * 100.0;
  lon = lonDeg + lonMin / 60.0;
  if (lonH[0] == 'W') lon = -lon;

  return true;
}

void connectGPS2IP() {
  if (gpsClient.connected()) return;

  IPAddress phoneIP = WiFi.gatewayIP();
  Serial.print("Connecting to GPS2IP at ");
  Serial.print(phoneIP);
  Serial.print(":");
  Serial.println(GPS2IP_PORT);

  gpsClient.stop();
  if (!gpsClient.connect(phoneIP, GPS2IP_PORT)) {
    Serial.println("GPS2IP connect failed; retrying...");
    delay(500);
    return;
  }
  Serial.println("GPS2IP connected!");
  lineBuf = "";
}

void populate_lat_lon(double& lat, double& lon) {
  connectGPS2IP();
  if (!gpsClient.connected()) {
    delay(50);
    return;
  }

  while (gpsClient.available()) {
    char c = (char)gpsClient.read();
    if (c == '\n') {
      String line = lineBuf;
      lineBuf = "";
      line.trim(); // removes \r too

      if (line.startsWith("$GPRMC,")) {
        // Optional: print raw RMC
        // Serial.println(line);
        if (parseRMC_latlon(line, lat, lon)) {
          Serial.print("Lat: ");
          Serial.print(lat, 7);
          Serial.print(", Lon: ");
          Serial.println(lon, 7);
        }
      }
    } else {
      if (lineBuf.length() < 200) lineBuf += c;
      else lineBuf = "";
    }
  }

  // if the phone drops the socket, reconnect next loop
  if (!gpsClient.connected()) {
    Serial.println("GPS2IP disconnected.");
    delay(200);
  }
}

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    *lane_deviation_info = pCharacteristic->getValue();
    digitalWrite(lane_deviation_BLE_LED, HIGH);
    received_lane_deviation = true;
  }
};

//Callback to retrieve data.
void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  
  for (int i = 0; i < length; i++) {
    rx_data += (char)pData[i];
  }

  received_response_data = true;
}


bool initOBD() {
  String init_command[6] = {"ATZ\r", "ATE0\r", "ATL0\r", "ATS0\r", "ATSP0\r", "ATST100"};
  for (int command_idx = 0; command_idx < 5; ++command_idx) {
    pMyRemoteWriteCharacteristic->writeValue(init_command[command_idx]);
    delay(700);

    if (!received_response_data) {
      Serial.print("Initialization failure: The current index is ");
      Serial.println(command_idx);
      return false;
    }
    received_response_data = false;
  }
  return true;
}

void Task1Code(void * pvParameters) {

  struct tm timeinfo;
  std::string data_to_analyze = "";
  String data_to_analyze_string = "";
  

  for(;;) { // Tasks must never return
    // Serial.println(F("Task 1 is running on Core 1"));        
    unsigned long start = millis();
    message = "";
   
    web_packet.time_with_date = "";

    if (setup_success) {
      if (!pMyClient->isConnected()) {
        String end_of_trip_message = "end_of_trip"; //FLAG
        sendHTTPData(end_of_trip_message, serverUrl);
        // Serial.println(message);
        digitalWrite(success_status_LED, LOW);
        digitalWrite(lane_deviation_BLE_LED, LOW);
        digitalWrite(send_to_data_base_LED, LOW);
        // return;
      } else {

        while (!getLocalTime(&timeinfo)) { //FLAG
          Serial.println(F("Failed to get local time"));
        }

        rx_data.clear();
        // Serial.print(timeinfo.tm_hour);
        web_packet.time_with_date += timeinfo.tm_hour;

        // Serial.print(":");
        web_packet.time_with_date += ":";
        if (timeinfo.tm_min < 10) {
          // Serial.print(0);
          web_packet.time_with_date += 0;
        }

        web_packet.time_with_date += timeinfo.tm_min;
        message += timeinfo.tm_min;
        // Serial.print(timeinfo.tm_min);
        // Serial.print(":");
        web_packet.time_with_date += ":";
        if (timeinfo.tm_sec < 10) {
          // Serial.print(0);
          web_packet.time_with_date += 0;
        }
        // Serial.print(timeinfo.tm_sec);
        web_packet.time_with_date += timeinfo.tm_sec;
        // Serial.print(", ");


        // Serial.print("Timestamp: ");
        // Serial.println(web_packet.time_with_date);

        std::string requests[1] = {SPEED};
        int num_requests = 1;
        OBD_Request_Handler obd_parser(requests, num_requests);

        pMyRemoteWriteCharacteristic->writeValue("010D\r");
        vTaskDelay(delayms / portTICK_PERIOD_MS);
        if (received_response_data) {

          int num_bytes_to_copy = (rx_data.length() - (2*num_requests + 2)) / 3;
          int starter_index = 2*num_requests + 2;
          for (int i = starter_index; i < starter_index + num_bytes_to_copy; i++) {
            data_to_analyze += rx_data[i];
          }

          for (int i = 0; i < data_to_analyze.length(); i++) {
            data_to_analyze_string += data_to_analyze[i];
          }

          int results[1];
          obd_parser.return_results(results, data_to_analyze);
          data_to_analyze = "";
          data_to_analyze_string.clear();
          web_packet.speed = results[0];
          // Serial.print("Speed:");
          // Serial.println(web_packet.speed);
          rx_data.clear();

          received_response_data = false; //reset flag.
        } else {
          rx_data.clear();
        }

        std::string new_requests[1] = {ACCELERATOR_PEDAL_POS_D};
        int num_requests_2 = 1;
        obd_parser.update_requests(new_requests, num_requests_2);
        pMyRemoteWriteCharacteristic->writeValue("0149\r");
        vTaskDelay(delayms / portTICK_PERIOD_MS);
        if (received_response_data) {
          int num_bytes_to_copy = (rx_data.length() - (2*num_requests + 2));
          int starter_index = 2*num_requests + 2;
          for (int i = starter_index; i < starter_index + num_bytes_to_copy; i++) {
            data_to_analyze += rx_data[i];
          }
          
          for (int i = 0; i < data_to_analyze.length(); i++) {
            data_to_analyze_string += data_to_analyze[i];
          }
          int results[1];
          obd_parser.return_results(results, data_to_analyze);
          data_to_analyze = "";
          data_to_analyze_string.clear();
          web_packet.acc_pedal = results[0];
          // Serial.print("Pedal: ");
          // Serial.println(web_packet.acc_pedal);
          rx_data.clear();
        } else {
          rx_data.clear();
        }

        //if we have received lane deviation info, then add it to the message.
        if (received_lane_deviation) {
          web_packet.lane_offset += *lane_deviation_info;
          web_packet.lane_offset_direction += *lane_deviation_info;
          Serial.print("Info is: ");
          Serial.println(*lane_deviation_info);
        } else {
          // Serial.print("EMPTY");      
        }
      }
    }


// We lock ONLY to copy the data, then release immediately.
    if (xSemaphoreTake(xDataMutex, portMAX_DELAY) == pdTRUE) {
        
      // Sync current state to our local snapshot
      // memcpy(&web_packet_local, &web_packet, sizeof(WebPacket));`
      Serial.println("The message is");
      construct_message(web_packet, &message);
      Serial.println(message);
      xSemaphoreGive(xDataMutex); 
    }

        // //set status LED.
    if (!sendHTTPData(message, serverUrl)) {
      digitalWrite(send_to_data_base_LED, LOW);
    } else {  //Successful message_sending attempt.
      digitalWrite(send_to_data_base_LED, HIGH);
    }

    webSocket.loop();
    unsigned long lastKeepAlive = 0;
    unsigned long currentTime = millis();

    if (currentTime - lastKeepAlive > 25000) {
      webSocket.sendTXT("keepalive");
      lastKeepAlive = currentTime;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void Task2Code(void* pvParameters) {
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(10));
    Serial.println("Task 2 is running on Core 2");
    sensors_event_t a;
    sensors_event_t g;
    sensors_event_t temp;
    mpu.getEvent(&a, &g, &temp);

    // Serial.println(g.gyro.heading);

    double latitude, longitude;
    populate_lat_lon(latitude, longitude);
    // Serial.print(F("The latitude is: "));
    // Serial.println(latitude);
    // Serial.print(F("The longitude is: "));
    // Serial.println(longitude);
    if (xSemaphoreTake(xDataMutex, 0) == pdTRUE) {
        web_packet.gps_longitude = longitude;
        web_packet.gps_latitude = latitude;
        web_packet.yaw = g.gyro.heading;
        web_packet.acceleration_x = a.acceleration.x;
        web_packet.acceleration_y = a.acceleration.y;
        web_packet.acceleration_z = a.acceleration.z;

        // web_packet.yaw = 0.0;
        // web_packet.acceleration_x = 0.1;
        // web_packet.acceleration_y = 0.2;
        // web_packet.acceleration_z = 0.3;
        xSemaphoreGive(xDataMutex); // Release immediately
    } else {
        // If the mutex was busy (only happens for ~2 microseconds), 
        // we skip this update to maintain our 10ms timing.
    }


    vTaskDelay(pdMS_TO_TICKS(100));;
  }
}

void setup() {
  Serial.begin(9600);

  xDataMutex = xSemaphoreCreateMutex(); 
  if (xDataMutex == NULL) {
    Serial.println("Mutex creation failed!");
    while(1); //Don't proceed if we can't create the mutex lmao.
  }
  
  if (!connectToWifi(WIFI_STA, ssid, password, 3, 5000)) {
    Serial.println("Connection to Wifi failed");
    return;
  }

  Serial.println("Response");
  Serial.println(WiFi.macAddress());

  //   // Start WebSocket
  webSocket.beginSSL(
      "driving-buddy-public-44aec6b920a3.herokuapp.com",
      443,
      "/websocky"
  );

  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  // pinMode(success_status_LED, OUTPUT);
  // pinMode(lane_deviation_BLE_LED, OUTPUT);
  // pinMode(send_to_data_base_LED, OUTPUT);
  // digitalWrite(success_status_LED, LOW);
  // digitalWrite(lane_deviation_BLE_LED, LOW);
  // digitalWrite(send_to_data_base_LED, LOW);

  const char* ntpServer = "pool.ntp.org";
  const long  gmtOffset_sec = -4 * 3600; // EDT (UTC-4)
  const int   daylightOffset_sec = 3600; // DST offset

  configTime(gmtOffset_sec, 0, ntpServer);

  // if (!getLocalTime(&timeinfo)) {
  //   Serial.println("Failed to obtain time");
  //   return;
  // }

  Serial.println("Starting BLE work!");

  // Initialize BLE with device name
  BLEDevice::init("Veepeak_Connect");
  BLEAddress obd_address("8c:de:52:de:a2:6f");

  //Create the server
  BLEServer* pMyServer = BLEDevice::createServer();
  BLEService* pMyService = pMyServer->createService(serviceUUID);   //create the service
  BLECharacteristic *pESP32Characteristics = pMyService->createCharacteristic(serverCharacteristicUUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  if (pESP32Characteristics == nullptr) { //if we can't get the characteristics...
    Serial.println("No server characteristics");
    return;
  }

  pESP32Characteristics->setCallbacks(new MyCallbacks());
  pMyService->start();  //start your service
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(serviceUUID);              
  BLEDevice::startAdvertising();

  //Create client. ESP32 is the client
  pMyClient = BLEDevice::createClient();
  if (!pMyClient->connect(obd_address)) { //connect to device
    // Serial.println("Failed to connect");
  } else {  //If we can connect to the scanner...
    String initialization_message = "start_of_trip";
    // initial_message(initialization_message)
    //START OF TRIP.
    while (!sendHTTPData(initialization_message, serverUrl)) {
      Serial.println("Failed to start trip");
      // return;
    }

    digitalWrite(send_to_data_base_LED, HIGH);
    delay(100);
    digitalWrite(send_to_data_base_LED, LOW);
    
  
    //send
    BLERemoteService* pMyRemoteService = pMyClient->getService(serviceUUID);
    if (pMyRemoteService == nullptr) {
      Serial.println("No service");
      return;
    }
    pMyRemoteReadCharacteristic = pMyRemoteService->getCharacteristic(readCharacteristicUUID);
    pMyRemoteWriteCharacteristic = pMyRemoteService->getCharacteristic(writeCharacteristicUUID);

    Serial.println("Successful connection to Veepeak");
    if (pMyRemoteReadCharacteristic == nullptr || pMyRemoteWriteCharacteristic == nullptr) {
      Serial.println("Failed to find characteristics.");
      return;
    }

      // Try to initialize!
    int num_retries = 0;
    if (!mpu.begin()) {
      // Serial.println("Failed to find MPU6050 chip");
      while (1) { delay(10); } // Stop here if sensor is missing
    }


    Serial.println("MPU6050 Found!");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

    Serial.println("");
    delay(100);

    //register the callback function.
    pMyRemoteReadCharacteristic->registerForNotify(notifyCallback);

    bool init_success = initOBD();
    if (init_success) {
      Serial.println("Setup Success");
      setup_success = true;
      digitalWrite(success_status_LED, HIGH);
      Serial.flush();
    } else {
      // Serial.println("Setup failure");
    }
  }

// Task 1: Main Logic (BLE + HTTP + OBD)
  // We put this on Core 1 to keep it away from the main WiFi/BLE system interrupts on Core 0.
  xTaskCreatePinnedToCore(
    Task1Code,          /* Function */
    "OBD_Task",         /* Name */
    5000,              /* Stack size (Words) - Increased for String safety */
    NULL,               /* Parameter */
    1,                  /* Priority - Standard */
    NULL,               /* Task handle */
    1);                 /* Core 1 */

  // Task 2: GPS & Serial Logging
  // We put this on Core 0. Since it's priority 1, it will "yield" to WiFi tasks when needed.
  xTaskCreatePinnedToCore(
    Task2Code, 
    "GPS_Task", 
    5000, 
    NULL, 
    2,                  /* Priority - Must match Task 1 for stability */
    NULL, 
    0);
}



void loop() {
  vTaskDelay(1000);
}
