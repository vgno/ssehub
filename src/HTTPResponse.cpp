#include <sstream>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "Common.h"
#include "HTTPResponse.h"

#define CRLF "\r\n"

HTTPResponse::HTTPResponse() {
  m_statusCode = 200;
  m_statusMsg = "OK";
  SetHeader("Content-Type", "text/html");
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

  if ((m_headers["Connection"].compare("close") == 0) && m_body.size() > 0)
    SetHeader("Content-Length", boost::lexical_cast<std::string>(m_body.size()));

  ss << "HTTP/1.1 " << m_statusCode << " " << m_statusMsg << CRLF;

  BOOST_FOREACH(HeaderList_t::value_type& header, m_headers) {
    ss << header.first << ": " << header.second << CRLF;
  }

  ss << CRLF << m_body;

  return ss.str();
}


