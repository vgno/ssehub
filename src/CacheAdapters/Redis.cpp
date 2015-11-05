#include "Common.h"
#include "CacheAdapters/Redis.h"
#include "SSEConfig.h"
#include "SSEEvent.h"
#include <string>
#include <vector>
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>

using namespace std;
extern int stop;


Redis::Redis(const string key, const ChannelConfig& config) : _config(config), _key(key) {
  _host = Lookup(_config.server->GetValue("redis.host"));
  _port = _config.server->GetValueInt("redis.port");

  if (_host.empty()) {
    LOG(ERROR) << "Failed to look up host for redis adapter " << _config.server->GetValue("redis.host") << " Retrying in 5 seconds.";
  }

  GetClient();
}

RedisSyncClient* Redis::GetClient() {
  boost::asio::ip::address address;
  boost::asio::io_service ioService;
  string errmsg;

  try {
    address = boost::asio::ip::address::from_string(_host);
  } catch(const runtime_error& error) {
    LOG(ERROR) << "Boost address lookup error: " << error.what();
  }

  RedisSyncClient* client = new RedisSyncClient(ioService);

  if(!client->connect(address, _port, errmsg)) {
    LOG(ERROR) << "Failed to connect to redis: " << errmsg << ". Host: " << _host << " Port: " << _port;

    return NULL;
  }
  return client;
}

void Redis::CacheEvent(SSEEvent* event) {
  RedisValue result;

  RedisSyncClient* client = GetClient();
  if (!client) {
    return;
  }

  try {
    result = client->command("HSET", _key, event->getid(), event->get());
  } catch (const runtime_error& error) {
    LOG(ERROR) << "Redis::CacheEvent: " << error.what();
  }

  if (result.isError()) {
    LOG(ERROR) << "SET error: " << result.toString();
  }

  result = client->command("HLEN", _key);
  if (result.isError()) {
    LOG(ERROR) << "HLEN error" << result.toString();
  }
  if (result.isOk()) {
    if (result.toInt() > _config.cacheLength) {
      result = client->command("HKEYS", _key);
      if (result.isError()) {
        LOG(ERROR) << "HKEYS error: " << result.toString();
      }
      if (result.isOk()) {
        client->command("HDEL", _key, result.toArray().front().toString());
      }
    }
  }
}

deque<string> Redis::GetEventsSinceId(string lastId) {
  deque<string> events;
  RedisValue result;

  RedisSyncClient* client = GetClient();
  if (!client) {
    return events;
  }

  try {
    result = client->command("HGETALL", _key);
  } catch (const runtime_error& error) {
    LOG(ERROR) << "Redis::GetEventsSinceId: " << error.what();
  }

  if (result.isError()) {
    LOG(ERROR) << "GET error: " << result.toString();
  }

  if (result.isOk() && result.isArray()) {
    std::vector<RedisValue> resultArray = result.toArray();
    string id = "";
    string data = "";
    while (!resultArray.empty()) {
      RedisValue eventResult = resultArray.back();
      if (eventResult.isString()) {
        if (data == "") {
          data = eventResult.toString();
        } else {
          id = eventResult.toString();
          if (id >= lastId) {
            events.push_front(data);
          }
          id = "";
          data = "";
        }
      }
      resultArray.pop_back();
    }
  }
  return events;
}

deque<string> Redis::GetAllEvents() {
  RedisValue result;
  deque<string> events;

  RedisSyncClient* client = GetClient();
  if (!client) {
    return events;
  }

  try {
    result = client->command("HGETALL", _key);
  } catch (const runtime_error& error) {
    LOG(ERROR) << "Redis::GetEventsSinceId: " << error.what();
  }

  if (result.isError()) {
    LOG(ERROR) << "GET error: " << result.toString();
  }

  if (result.isOk() && result.isArray()) {
    std::vector<RedisValue> resultArray = result.toArray();
    string data = "";
    while (!resultArray.empty()) {
      RedisValue eventResult = resultArray.back();
      if (eventResult.isString()) {
        if (data == "") {
          data = eventResult.toString();
        } else {
          events.push_front(data);
          data = "";
        }
      }
      resultArray.pop_back();
    }
  }
  return events;
}

int Redis::GetSizeOfCachedEvents() {
  int size = 0;
  RedisValue result;

  RedisSyncClient* client = GetClient();
  if (!client) {
    return size;
  }

  try {
    result = client->command("HLEN", _key);
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
