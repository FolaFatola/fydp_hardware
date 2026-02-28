#ifndef MISCELLANEOUS_HPP
#define MISCELLANEOUS_HPP

#include "Arduino.h"

typedef struct WebPacket {
  String userID;
  // String tripID;
  String time_with_date;
  float speed;
  float acc_pedal;
  float acceleration_x;
  float acceleration_y;
  float acceleration_z;
  float yaw;
  float gps_latitude;
  float gps_longitude;
  String lane_offset;
  String lane_offset_direction;
} WebPacket;


void construct_message(WebPacket packet, String* message);


#endif