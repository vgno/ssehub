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

Redis::Redis(string key, int length) {
    _key = key;
    _config.length = length;

    Connect();
}

void Redis::Connect() {
  boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
  const unsigned short port = 6379;

  boost::asio::io_service ioService;
  _client = new RedisSyncClient(ioService);
  string errmsg;

  if(!_client->connect(address, port, errmsg)) {
    LOG(ERROR) << "Failed to connect to redis:" << errmsg << "\n retrying in 5 seconds.";
    Reconnect(5);
    return;
  }

  LOG(INFO) << "Connected to redis";
}

void Redis::Disconnect() {
  // TODO: Gracefully handle disconnects
}

void Redis::Reconnect(int delay) {
  Disconnect();
  sleep(delay);
  Connect();
}

void Redis::CacheEvent(SSEEvent* event) {
  RedisValue result = _client->command("HSET", _key, event->getid(), event->get());
  if (result.isError()) {
    LOG(ERROR) << "SET error: " << result.toString();
  }

  result = _client->command("HLEN", _key);
  if (result.isError()) {
    LOG(ERROR) << "HLEN error" << result.toString();
  }
  if (result.isOk()) {
    if (result.toInt() > _config.length) {
      result = _client->command("HKEYS", _key);
      if (result.isError()) {
        LOG(ERROR) << "HKEYS error: " << result.toString();
      }
      if (result.isOk()) {
        _client->command("HDEL", _key, result.toArray().front().toString());
      }
    }
  }
}

deque<string> Redis::GetEventsSinceId(string lastId) {
  deque<string> events;
  RedisValue result;

  result = _client->command("HGETALL", _key);
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
  deque<string> events;

  RedisValue result = _client->command("HGETALL", _key);
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
  RedisValue result = _client->command("HLEN", _key);
  if (result.isError()) {
    LOG(ERROR) << "GET error: " << result.toString();
  }
  if (result.isOk()) {
    size = result.toInt();
  }
  return size;
}
