#include "HTTPRequest.h"
#include <iostream>

HTTPRequest::HTTPRequest(const char *data, int len) {
  Parse(data, len);
}

HTTPRequest::~HTTPRequest() {

}

bool HTTPRequest::Parse(const char *data, int len) {
  const char *phr_method, *phr_path;
  struct phr_header phr_headers[HTTP_REQUEST_MAX_HEADERS];
  size_t phr_num_headers, phr_method_len, phr_path_len;
  int pret, phr_minor_version, i;
     
  phr_num_headers = sizeof(phr_headers) / sizeof(phr_headers[0]);

  pret = phr_parse_request(data, len, &phr_method, &phr_method_len, &phr_path, 
      &phr_path_len, &phr_minor_version, phr_headers, &phr_num_headers, 0);    

  if (pret < 1){
    b_success = false;
    return false;
  }

  if (phr_path_len > 0)
    path.insert(0, phr_path, phr_path_len);

  if (phr_method_len > 0)
    method.insert(0, phr_method, phr_method_len);

  for (i = 0; i < (int)phr_num_headers; i++) {
    string name, value; 
    name.insert(0, phr_headers[i].name, phr_headers[i].name_len);
    value.insert(0, phr_headers[i].value, phr_headers[i].value_len);
    headers[name] = value;
  }

  return true;
} 

const string& HTTPRequest::GetPath() {
  return path;
}

const string& HTTPRequest::GetMethod() {
  return method;
}


const map<string, string>& HTTPRequest::GetHeaders() {
  return headers;
}

bool HTTPRequest::Success() {
  return b_success;
}
