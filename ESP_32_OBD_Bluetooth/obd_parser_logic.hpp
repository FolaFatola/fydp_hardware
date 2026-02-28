#ifndef OBD_PARSER_LOGIC_HPP
#define OBD_PARSER_LOGIC_HPP

#include <string>
#include "math.h"
#include <Arduino.h>

#define SPEED "0C"
#define RPM "0D"
#define ENGINE_LOAD "04"
#define THROTTLE_POS_B "47"
#define THROTTLE_POS_C "48" //changed
#define ACCELERATOR_PEDAL_POS_D "49"
#define ACCELERATOR_PEDAL_POS_E "4A"
#define ACCELERATOR_PEDAL_POS_F "4B"
#define COMMAND_THROTTLE_ACTUATOR "4C"


#define CURRENT_INFORMATION "01"

//list more obd2 commands here.

class OBD_Request_Handler {
public:
  OBD_Request_Handler(std::string *requests, int num_requests);

  //Creates requests to send to the OBD scanner.
  char *construct_request();
  void update_requests(std::string *requests, int num_requests);
  
  int skip_obd_meta_data(std::string response);
  float extract_pid_result(std::string pid);
  void return_results(int *obd2_results, std::string response);
  

private:
  std::string *requests_;      //array of obd2 request pids
  std::string obd2_response_;   //Obd2 response string
  char request_to_write_[100];

  int num_reqs_;          //number of requests you want to send in parallel.
  int *results_;          //array of numerical results
  int result_str_idx_;    //index currently being parsed in the obd2 response string

  int convert_hex_to_int(std::string hex);
  int fromChartoInt(char ch);
  std::string extract(int num_chars_to_extract);
};  

#endif
