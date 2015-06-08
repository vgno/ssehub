#include "string.h"
#include "HTTPRequest.h"
#include <glog/logging.h>

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
    // Assume there is a querystring present if the URI contains a ?.
    const char* qstr_pos = strchr((const char*)phr_path, '?');
    if (qstr_pos != NULL) { // URI contains a ?
      // Calculate the actual length of the path and query string. 
      size_t path_len = qstr_pos - phr_path;
      size_t qstr_len = (phr_path_len - path_len) - 1;

      // Copy the path into path variable.
      path.insert(0, phr_path, path_len);

      // Copy the querystring into a variable.
      string qstr;
      qstr.insert(0, qstr_pos+1, qstr_len);

      // Parse the querystring (+1 to skip the ?).
      ParseQueryString(qstr);
    } else {
      // No querystring present - just copy the whole path.
      path.insert(0, phr_path, phr_path_len);
    }
  }

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
const string& HTTPRequest::GetHeader(const string& header) {
  return headers[header];
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
      qsmap[param] = val;
    }
  }

  return qsmap.size();
}

/**
  Get a spesific query string parameter.
  @param param Parameter to get.
**/
const string& HTTPRequest::GetQueryString(const std::string& param) {
  return qsmap[param];
}

/**
  Returns number of query strings in the request.
**/
size_t HTTPRequest::NumQueryString() {
  return qsmap.size();
}
