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
    Connect();
}

void Redis::Connect() {
  string host = Lookup(_config.server->GetValue("redis.host"));
  const unsigned short port = _config.server->GetValueInt("redis.port");
  const unsigned int NUM_CONNECTIONS = _config.server->GetValueInt("redis.numConnections");

  if (host.empty()) {
    LOG(ERROR) << "Failed to look up host for redis adapter " << _config.server->GetValue("redis.host") << " Retrying in 5 seconds.";
    Reconnect(5);
    return;
  }

  boost::asio::ip::address address = boost::asio::ip::address::from_string(host);
  boost::asio::io_service ioService;
  string errmsg;

  LOG(INFO) << "Creating redis pool of " << _config.server->GetValue("redis.numConnections") << " connections.";

  for (unsigned int i = 0; i < NUM_CONNECTIONS; i++) {
    RedisSyncClient* client = new RedisSyncClient(ioService);

    if(client->connect(address, port, errmsg)) {
      _clients.push_back(RedisSyncClientPtr(client));
      LOG(INFO) << "Connected to redis server " << _config.server->GetValue("redis.host") << ":" << _config.server->GetValue("redis.port");
    } else {
      LOG(ERROR) << "Failed to connect to redis:" << errmsg << ". Retrying in 5 seconds.";
      Reconnect(5);
      return;
    }
  }

  _curclient = _clients.begin();
}

void Redis::Disconnect() {
  while(_clients.size() > 0) {
    _clients.pop_back();
  }
}

void Redis::Reconnect(int delay) {
  if (!stop) {
    Disconnect();
    sleep(delay);
    Connect();
  }
}

RedisSyncClient* Redis::GetClient() {
  RedisSyncClient* client = (*_curclient).get();
  _curclient++;
  if (_curclient == _clients.end()) _curclient = _clients.begin();

  return client;
}

void Redis::CacheEvent(SSEEvent* event) {
  RedisValue result;
  try {
    result = GetClient()->command("HSET", _key, event->getid(), event->get());
  } catch (const runtime_error& error) {
    LOG(ERROR) << "Redis::CacheEvent: " << error.what();
  }

  if (result.isError()) {
    LOG(ERROR) << "SET error: " << result.toString();
  }

  result = GetClient()->command("HLEN", _key);
  if (result.isError()) {
    LOG(ERROR) << "HLEN error" << result.toString();
  }
  if (result.isOk()) {
    if (result.toInt() > _config.cacheLength) {
      result = GetClient()->command("HKEYS", _key);
      if (result.isError()) {
        LOG(ERROR) << "HKEYS error: " << result.toString();
      }
      if (result.isOk()) {
        GetClient()->command("HDEL", _key, result.toArray().front().toString());
      }
    }
  }
}

deque<string> Redis::GetEventsSinceId(string lastId) {
  deque<string> events;
  RedisValue result;

  try {
    result = GetClient()->command("HGETALL", _key);
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

  try {
    result = GetClient()->command("HGETALL", _key);
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

  try {
    result = GetClient()->command("HLEN", _key);
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
