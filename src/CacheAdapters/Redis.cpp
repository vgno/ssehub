#include "Common.h"
#include "CacheAdapters/Redis.h"
#include "SSEConfig.h"
#include "SSEEvent.h"
#include <string>
#include <vector>
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/foreach.hpp>

using namespace std;
extern int stop;


Redis::Redis(const string key, const ChannelConfig& config) : _config(config) {
  _host = Lookup(_config.server->GetValue("redis.host"));
  _port = _config.server->GetValueInt("redis.port");
  _key = _config.server->GetValue("redis.prefix") + "_" + key;


  if (_host.empty()) {
    LOG(ERROR) << "Failed to look up host for redis adapter " << _config.server->GetValue("redis.host") << " Retrying in 5 seconds.";
  }
}

bool Redis::InitClient(RedisSyncClient& client)  {
  boost::asio::ip::address address;
  string errmsg;

  try {
    address = boost::asio::ip::address::from_string(_host);
  } catch(const runtime_error& error) {
    LOG(ERROR) << "Boost address lookup error: " << error.what();
  }

  if(!client.connect(address, _port, errmsg)) {
    LOG(ERROR) << "Failed to connect to redis: " << errmsg << ". Host: " << _host << " Port: " << _port;
    return false;
  }

  return true;
}

void Redis::CacheEvent(SSEEvent* event) {
  RedisValue result;
  boost::asio::io_service ioService;
  RedisSyncClient client(ioService);

  if (!InitClient(client)) {
    return;
  }

  try {
    result = client.command("HSET", _key, event->getid(), event->get());
  } catch (const runtime_error& error) {
    LOG(ERROR) << "Redis::CacheEvent: " << error.what();
  }

  if (result.isError()) {
    LOG(ERROR) << "SET error: " << result.toString();
  }

  result = client.command("HLEN", _key);
  if (result.isError()) {
    LOG(ERROR) << "HLEN error" << result.toString();
  }
  if (result.isOk()) {
    if (result.toInt() > _config.cacheLength) {
      result = client.command("HKEYS", _key);
      if (result.isError()) {
        LOG(ERROR) << "HKEYS error: " << result.toString();
      }
      if (result.isOk()) {
        client.command("HDEL", _key, result.toArray().front().toString());
      }
    }
  }
}

deque<string> Redis::GetEventsSinceId(string lastId) {
  deque<string> events;
  RedisValue result;
  boost::asio::io_service ioService;
  RedisSyncClient client(ioService);

  if (!InitClient(client)) {
    return events;
  }

  try {
    result = client.command("HGETALL", _key);
  } catch (const runtime_error& error) {
    LOG(ERROR) << "Redis::GetEventsSinceId: " << error.what();
  }

  if (result.isError()) {
    LOG(ERROR) << "GET error: " << result.toString();
  }

  if (result.isOk() && result.isArray()) {
    std::vector<RedisValue> resultArray = result.toArray();

    bool isId = true;
    bool ignoreEvent = true;
    BOOST_FOREACH(const RedisValue& value, resultArray) {
      if (value.isString() && value.toString().length() > 0) {
        if (isId) {
          if (value.toString().compare(lastId) == 0) {
            ignoreEvent = false;
          }
        } else if (!ignoreEvent){
          events.push_back(value.toString());
        }

        isId = !isId;
      }
    }
  }

  return events;
}

deque<string> Redis::GetAllEvents() {
  RedisValue result;
  deque<string> events;
  boost::asio::io_service ioService;
  RedisSyncClient client(ioService);

  if (!InitClient(client)) {
    return events;
  }

  try {
    result = client.command("HGETALL", _key);
  } catch (const runtime_error& error) {
    LOG(ERROR) << "Redis::GetEventsSinceId: " << error.what();
  }

  if (result.isError()) {
    LOG(ERROR) << "GET error: " << result.toString();
  }

  if (result.isOk() && result.isArray()) {
    std::vector<RedisValue> resultArray = result.toArray();

    bool isId = true;
    BOOST_FOREACH(const RedisValue& value, resultArray) {
      if (value.isString() && value.toString().length() > 0) {
        if (!isId) {
          string data = value.toString();
          events.push_back(data);
        }

        isId = !isId;
      }
    }
  }

  return events;
}

int Redis::GetSizeOfCachedEvents() {
  int size = 0;
  RedisValue result;
  boost::asio::io_service ioService;
  RedisSyncClient client(ioService);

  if (!InitClient(client)) {
    return size;
  }

  try {
    result = client.command("HLEN", _key);
  } catch (const runtime_error& error) {
    LOG(ERROR) << "Redis::GetEventsSinceId: " << error.what();
  }

  if (result.isError()) {
    LOG(ERROR) << "GET error: " << result.toString();
  }

  if (result.isOk()) {
    size = result.toInt();
  }

  return size;
}

string Redis::Lookup(string hostname) {
  hostent * record = gethostbyname(hostname.c_str());

  if(record == NULL) {
    return "";
  }

  in_addr * address = (in_addr * )record->h_addr;
  string ip_address = inet_ntoa(* address);

  return ip_address;
}
