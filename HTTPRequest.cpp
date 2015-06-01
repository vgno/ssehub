#include "HTTPRequest.h"

HTTPRequest::HTTPRequest(const char *data, int len) {
  const char *method, *path;
  struct phr_header headers[50];
  size_t nheaders, method_len, path_len;
  int pret, minor_version;
     
  nheaders = sizeof(headers) / sizeof(headers[0]);
  pret = phr_parse_request(data, len, &method, &method_len, &path, &path_len, &minor_version, headers, &nheaders, 0);    
}
