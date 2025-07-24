#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Pixel_6862";
const char* password = "Ece@498pass";
String serverUrl = "http://sneha.ca:80/esp32";

//connecting to wifi
bool connectToWifi(wifi_mode_t mode,  const char* ssid, const char* password, int num_tries, int timeOutMs) {
    int connection_status = 0;
    int trial_number = 0;

    WiFi.mode(mode);

    connection_status = WiFi.waitForConnectResult(timeOutMs);
    
    while (trial_number < num_tries) {
      WiFi.begin(ssid, password);
      connection_status = WiFi.waitForConnectResult(timeOutMs);
      if (connection_status == WL_CONNECTED) {
        break;
      }
      trial_number++;
      Serial.print("Retrying connection....\n");
    }

    if (connection_status != WL_CONNECTED) {
      return false;
    }

    Serial.println("Connection Successful\n");
    return true;
}

//sending data to server
bool sendHTTPData(String message, String serverURL) {
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "text/plain");
  
  int httpResponseCode = http.POST(message);
  if (httpResponseCode != 200) {
    return false;
  } 

  http.end();

  return true;
}


void setup() {
  Serial.begin(9600);
  Serial.println(connectToWifi(WIFI_STA, ssid, password, 3, 10000));
}

void loop() {

  String message = "Hello, Sneha! This is Fola!";
  if (!sendHTTPData(message, serverUrl)) {
    Serial.println("Unsuccessful message send attempt\n");
  } else {
    Serial.println("Successful message attempt\n");
  }
  
}
