#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <map>
#include "../lib/picohttpparser/picohttpparser.h"

#define HTTPREQ_BUFSIZ 4096
#define HTTP_REQUEST_MAX_HEADERS 100

using namespace std;

enum HttpReqStatus {
  HTTP_REQ_FAILED,
  HTTP_REQ_INCOMPLETE,
  HTTP_REQ_OK
};

class HTTPRequest {
  public:
    HTTPRequest();
    ~HTTPRequest();
    HttpReqStatus Parse(const char *data, int len);
    bool Success();
    const string& GetPath();
    const string& GetMethod();
    const map<string, string>& GetHeaders();
    const string GetHeader(string header);
    const string GetQueryString(string param);
    size_t NumQueryString();

  private:
    int http_minor_version;
    bool b_success;
    std::string httpReq_buf;
    int httpReq_bytesRead;
    int httpReq_bytesReadPrev;
    bool httpReq_isComplete;

    const char *phr_method, *phr_path;
    struct phr_header phr_headers[HTTP_REQUEST_MAX_HEADERS];
    size_t phr_num_headers, phr_method_len, phr_path_len;
    int phr_minor_version;

    string path;
    string method;
    map<string, string> headers;
    map<string, string> query_parameters;
    map<string, string> qsmap;
    size_t ParseQueryString(const std::string& buf);
};

#endif
