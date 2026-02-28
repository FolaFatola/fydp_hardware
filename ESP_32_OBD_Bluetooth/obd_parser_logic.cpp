#include "obd_parser_logic.hpp"

char *OBD_Request_Handler::construct_request() {
    if (num_reqs_ <= 0) {
        return "\0";
    }

    int request_index = 0;
    request_to_write_[request_index] = '0';
    request_index++;
    request_to_write_[request_index] = '1';
    request_index++;

    for (int i = 0; i < num_reqs_; i++) {
        request_to_write_[request_index] = requests_[i][0];
        request_index++;
        request_to_write_[request_index] = requests_[i][1];
        request_index++;
        if (i  == num_reqs_ - 1) {
          request_to_write_[request_index] = '\r';
        }
        request_index++;
    }

    return request_to_write_;
}

OBD_Request_Handler::OBD_Request_Handler(std::string *requests, int num_requests) {
  requests_ = requests;
  num_reqs_ = num_requests;
  result_str_idx_ = 0;
  obd2_response_ = "";

  for (int i = 0; i < 100; i++) {
    request_to_write_[i] = '\0';
  }
}

void OBD_Request_Handler::update_requests(std::string *requests, int num_requests) {
  requests_ = requests;
  num_reqs_ = num_requests;
  result_str_idx_ = 0;
  obd2_response_ = "";

  for (int i = 0; i < 100; i++) {
    request_to_write_[i] = '\0';
  }
}

int OBD_Request_Handler::fromChartoInt(char ch) {
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

int OBD_Request_Handler::convert_hex_to_int(std::string pid) {
  int result = 0;
  for (int i = 0; i < pid.length(); i++) {
    result += fromChartoInt(pid[i]) * pow(16, pid.length() - 1 - i);
  }
  return result;
}


std::string OBD_Request_Handler::extract(int num_chars_to_extract) {
  result_str_idx_ += 2; //skip the requested pid code.
  //4928
  int extracted_chars = 0;
  std::string extracted_result = "";
  while (extracted_chars < num_chars_to_extract) {
    extracted_result += obd2_response_[result_str_idx_];
    extracted_chars += 1;
    result_str_idx_ += 1;
  }

  return extracted_result;
}


float OBD_Request_Handler::extract_pid_result(std::string pid) {
  float result = 0;
  std::string extracted_result = "";
  //0C0C790D00

    if (pid == SPEED) {
      extracted_result = extract(2);
      result = 16 * fromChartoInt(extracted_result[0]) + fromChartoInt(extracted_result[1]);
    } else if (pid == RPM) {
      extracted_result = extract(4);
      int A = fromChartoInt(extracted_result[0]) * 16 + fromChartoInt(extracted_result[1]);
      int B = fromChartoInt(extracted_result[2]) * 16 + fromChartoInt(extracted_result[3]);
      result = ((A * 256) + B) / 4;
    } else if (pid == ENGINE_LOAD) {
      extracted_result = extract(2);
      result = 16.0 * fromChartoInt(extracted_result[0]) + fromChartoInt(extracted_result[1]);
      result = (float) result / 2.55;
    } else if (pid == THROTTLE_POS_B || pid == THROTTLE_POS_C || pid == ACCELERATOR_PEDAL_POS_D || pid == ACCELERATOR_PEDAL_POS_E || pid == ACCELERATOR_PEDAL_POS_F || pid == COMMAND_THROTTLE_ACTUATOR) {
      extracted_result = extract(2);
      result = 16.0 * fromChartoInt(extracted_result[0]) + fromChartoInt(extracted_result[1]);
      result = (float) result / 2.55;
    } else {
      result = 0.133;
    }
      
  return result;
}

void OBD_Request_Handler::return_results(int *obd2_results, std::string response) {
  //clear the spaces
  for (char c : response) {
    if (c != ' ' && c != '\r' && c != '>') {
      obd2_response_ += c;
    }
  }
  //410C0C790D00
  //414928
  result_str_idx_ += 2; //do this to skip the meta data

  for (int requested_data_index = 0; requested_data_index < num_reqs_; ++requested_data_index) {
    //for each data response, you want to parse its data.
    //feed the request as a parameter when extracting the results. i.e., when searching for speed, the speed value is input as a parameter.
    obd2_results[requested_data_index] = extract_pid_result(requests_[requested_data_index]);
    // Serial.print("Request is:");
    // Serial.println(requests_[requested_data_index])
    
  }
  result_str_idx_ = 0;
}


