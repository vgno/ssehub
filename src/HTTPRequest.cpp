#include <glog/logging.h>
#include <string.h>
#include <boost/algorithm/string.hpp>
#include "HTTPRequest.h"

/**
  Constructor.
**/
HTTPRequest::HTTPRequest(const char *data, int len) {
  b_success = true;
  Parse(data, len);
}

/**
  Destructor.
**/
HTTPRequest::~HTTPRequest() {

}

/**
  Parse the request.
  @param data Raw http request data.
  @param len Length of data.
**/
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

  if (phr_path_len > 0) {
    string rawPath;
    rawPath.insert(0, phr_path, phr_path_len);

    size_t qsPos = rawPath.find_first_of('?', 0);
    if (qsPos != string::npos) {
      string qStr;
      qStr = rawPath.substr(qsPos+1, string::npos);
      path = rawPath.substr(0, qsPos);
      ParseQueryString(qStr);
    } else {
      path = rawPath.substr(0, rawPath.find_last_of(' ', 0));
    }
  }

  if (phr_method_len > 0)
    method.insert(0, phr_method, phr_method_len);

  for (i = 0; i < (int)phr_num_headers; i++) {
    string name, value; 
    name.insert(0, phr_headers[i].name, phr_headers[i].name_len);
    value.insert(0, phr_headers[i].value, phr_headers[i].value_len);
    boost::to_lower(name);
    headers[name] = value;
  }

  return true;
} 

/**
  Get the HTTP request path.
**/
const string& HTTPRequest::GetPath() {
  return path;
}

/**
  Get the HTTP request method.
**/
const string& HTTPRequest::GetMethod() {
  return method;
}

/**
  Get a spesific header.
  @param header Header to get.
**/
const string HTTPRequest::GetHeader(string header) {
  boost::to_lower(header);

  if (headers.find(header) != headers.end()) {
    return headers[header];
  }

  return "";
}

const map<string, string>& HTTPRequest::GetHeaders() {
  return headers;
}

/**
  Returns true if the request was successful and false otherwise.
**/
bool HTTPRequest::Success() {
  return b_success;
}

/**
  Extracts query parameters from a string if they exist.
  @param buf The string to parse.
**/
size_t HTTPRequest::ParseQueryString(const std::string& buf) {
  size_t prevpos = 0, eqlpos = 0;

  while((eqlpos = buf.find("=", prevpos)) != string::npos) {
    string param, val;
    size_t len;

    len = buf.find("&", eqlpos);

    if (len != string::npos) len = (len - eqlpos);
    else len = (buf.size() - eqlpos);

    param = buf.substr(prevpos, (eqlpos - prevpos));

    eqlpos++;
    val = buf.substr(eqlpos, len-1);
    prevpos = eqlpos+len;

    if (!param.empty() && !val.empty()) {
      boost::to_lower(param);
      qsmap[param] = val;
    }
  }

  return qsmap.size();
}

/**
  Get a spesific query string parameter.
  @param param Parameter to get.
**/
const string HTTPRequest::GetQueryString(string param) {
  boost::to_lower(param);

  if (qsmap.find(param) != qsmap.end()) {
    return qsmap[param];
  }

  return "";
}

/**
  Returns number of query strings in the request.
**/
size_t HTTPRequest::NumQueryString() {
  return qsmap.size();
}
