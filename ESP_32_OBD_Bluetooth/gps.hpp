#ifndef GPS_HPP
#define GPS_HPP

#include <HardwareSerial.h>
#include <WiFi.h>

// static const uint16_t GPS2IP_PORT = 11123;

// bool parseRMC_latlon(const String& line, double& lat, double& lon);
// void connectGPS2IP(String *lineBuf, WiFiClient *gpsClient);
// void fetch_lat_and_lon(double& lat, double& lon, String *lineBuf, WiFiClient* gpsClient);

class Neo6M_GPS {
public:
 Neo6M_GPS(int baud_rate, int RX_Pin, int TX_Pin);

private:
  int longitude;
  int latitude;
};

#endif
