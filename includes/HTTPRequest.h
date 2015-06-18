#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <map>
#include "../lib/picohttpparser/picohttpparser.h"

#define HTTP_REQUEST_MAX_HEADERS 100

using namespace std;

class HTTPRequest {
  public:
    HTTPRequest(const char *data, int len);
    ~HTTPRequest();
    bool Parse(const char *data, int len);
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
    string path;
    string method;
    map<string, string> headers;
    map<string, string> query_parameters;
    map<string, string> qsmap;
    size_t ParseQueryString(const std::string& buf);
};

#endif
