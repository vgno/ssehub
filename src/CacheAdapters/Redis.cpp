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

}

void Redis::Reconnect(int delay) {
  Disconnect();
  sleep(delay);
  Connect();
}

void Redis::CacheEvent(SSEEvent* event) {
  RedisValue result;
  result = _client->command("ZADD", _key, event->getid(), event->get());
  if (result.isError()) {
    LOG(ERROR) << "SET error: " << result.toString();
  }
}

deque<string> Redis::GetEventsSinceId(string lastId) {
  deque<string> events;
  RedisValue result;

  result = _client->command("ZRANGE", _key, lastId, "-1");
  if (result.isError()) {
    LOG(ERROR) << "SET error: " << result.toString();
  }
  

  return events;
}

deque<string> Redis::GetAllEvents() {
  deque<string> events;

  return events;
}

int Redis::GetSizeOfCachedEvents() {
    return (int) 0;
}
