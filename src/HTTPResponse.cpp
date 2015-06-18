#include <sstream>
#include <boost/foreach.hpp>
#include "HTTPResponse.h"

#define CRLF "\r\n"

HTTPResponse::HTTPResponse() {
  m_statusCode = 200;
  m_statusMsg = "OK";
  SetHeader("Connection", "close");
}

void HTTPResponse::SetStatus(int status, std::string statusMsg) {
  m_statusCode = status;
  m_statusMsg = statusMsg; 
}

void HTTPResponse::SetHeader(const std::string& name, const std::string& value) {
  m_headers[name] = value;
}

void HTTPResponse::SetBody(const std::string& data) {
  m_body = data;
}

void HTTPResponse::AppendBody(const std::string& data) {
  m_body.append(data);
}

const std::string HTTPResponse::Get() {
  std::stringstream ss; 

  ss << "HTTP/1.1 " << m_statusCode << " " << m_statusMsg << CRLF;

  BOOST_FOREACH(HeaderList_t::value_type& header, m_headers) {
    ss << header.first << ": " << header.second << CRLF;  
  }

  ss << CRLF << m_body;

  return ss.str(); 
}


