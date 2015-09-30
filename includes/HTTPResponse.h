#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <string>
#include <map>

typedef std::map<std::string, std::string> HeaderList_t;

class HTTPResponse {
  public:
    HTTPResponse(int statusCode=200, bool close=true);
    void SetStatus(int status, std::string statusMsg);
    void SetHeader(const std::string& name, const std::string& value);
    void SetBody(const std::string& data);
    void AppendBody(const std::string& data);
    const std::string GetStatusMsg(int statusCode);
    const std::string Get();

  private:
    int m_statusCode;
    std::string m_statusMsg;
    std::string m_body;
    HeaderList_t m_headers;
};

#endif
