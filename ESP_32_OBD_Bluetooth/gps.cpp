#include "gps.hpp"

// void connectGPS2IP(String *lineBuf, WiFiClient *gpsClient) {
//   if (gpsClient->connected()) return;

//   IPAddress phoneIP = WiFi.gatewayIP();
//   Serial.print("Connecting to GPS2IP at ");
//   Serial.print(phoneIP);
//   Serial.print(":");
//   Serial.println(GPS2IP_PORT);

//   gpsClient->stop();
//   if (!gpsClient->connect(phoneIP, GPS2IP_PORT)) {
//     Serial.println("GPS2IP connect failed; retrying...");
//     delay(500);
//     return;
//   }
//   Serial.println("GPS2IP connected!");
//   *lineBuf = "";
// }

// // parse
// bool parseRMC_latlon(const String& line, double& lat, double& lon) {
//   if (!(line.startsWith("$GPRMC,") || line.startsWith("$GNRMC,"))) return false;

//   int p[7];
//   int idx = 0;

//   for (int i = 0; i < line.length() && idx < 7; i++) {
//     if (line[i] == ',') {
//       p[idx++] = i;
//     }
//   }
//   if (idx < 7) return false;
  
//   // status field
//   String status = line.substring(p[1] + 1, p[2]);
//   if (status != "A") return false;  // only valid fixes

//   String latDM  = line.substring(p[2] + 1, p[3]);
//   String latH   = line.substring(p[3] + 1, p[4]);
//   String lonDM  = line.substring(p[4] + 1, p[5]);
//   String lonH   = line.substring(p[5] + 1, p[6]);
  
//   if (latDM.length() == 0 || lonDM.length() == 0) return false;
//   if (latH.length() == 0 || lonH.length() == 0) return false;

//   // convert DDMM.MMMMM → decimal
//   double latRaw = latDM.toFloat();
//   int latDeg = (int)(latRaw / 100.0);
//   double latMin = latRaw - latDeg * 100.0;
//   lat = latDeg + latMin / 60.0;
//   if (latH[0] == 'S') lat = -lat;

//   double lonRaw = lonDM.toFloat();
//   int lonDeg = (int)(lonRaw / 100.0);
//   double lonMin = lonRaw - lonDeg * 100.0;
//   lon = lonDeg + lonMin / 60.0;
//   if (lonH[0] == 'W') lon = -lon;

//   return true;
// }

// void fetch_lat_and_lon(double& lat, double& lon, String *lineBuf, WiFiClient* gpsClient) {
//   connectGPS2IP(lineBuf, gpsClient);
//   if (!gpsClient->connected()) {
//     delay(50);
//     return;
//   }

//   while (gpsClient->available()) {
//     char c = (char)gpsClient->read();
//     if (c == '\n') {
//       String line = *lineBuf;
//       *lineBuf = "";
//       line.trim(); // removes \r too

//       if (line.startsWith("$GPRMC,")) {
//         double lat, lon;
//         if (parseRMC_latlon(line, lat, lon)) {
//           // Serial.print("Lat: ");
//           // Serial.print(lat, 7);
//           // Serial.print(", Lon: ");
//           // Serial.println(lon, 7);
//         }
//       }
//     } else {
//       if (lineBuf->length() < 200) *lineBuf += c;
//       else *lineBuf = "";
//     }
//   }

//   // if the phone drops the socket, reconnect next loop
//   if (!gpsClient->connected()) {
//     Serial.println("GPS2IP disconnected.");
//     delay(10);
//   }
// }