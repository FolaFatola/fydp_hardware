#include "miscellaneous.hpp"

void construct_message(WebPacket packet, String* message) {
  *message = "";
  *message += "SomeID";
  *message += ", ";
  // *message += packet.tripID;
  // *message += ", ";
  *message += packet.time_with_date;
  *message += ", ";
  *message += packet.speed;
  *message += ", ";
  *message += packet.acc_pedal;
  *message += ", ";
  *message += packet.acceleration_x;
  *message += ", ";
  *message += packet.acceleration_y;
  *message += ", ";
  *message += packet.acceleration_z;
  *message += ", ";
  *message += packet.yaw;
  *message += ", ";
  *message += packet.gps_longitude;
  *message += ", ";
  *message += packet.gps_latitude;
  *message += ", ";
  *message += packet.lane_offset;
  *message += ", ";
  *message += packet.lane_offset_direction;
}