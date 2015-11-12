#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SSEChannelTest
#include <boost/test/unit_test.hpp>

#include "SSEChannel.cpp"
#include "SSEConfig.cpp"

#include "CacheAdapters/Memory.cpp"
#include "CacheAdapters/Redis.cpp"
#include "CacheAdapters/LevelDB.cpp"

#include "HTTPRequest.cpp"
#include "HTTPResponse.cpp"
#include "SSEEvent.cpp"
#include "SSEClient.cpp"
#include "SSEClientHandler.cpp"
#include "../lib/picohttpparser/picohttpparser.c"

int stop = 0;

BOOST_AUTO_TEST_CASE(canConstructSSEChannel) {
  SSEConfig* serverConfig = new SSEConfig();
  ChannelConfig channelConfig;
  channelConfig.server = serverConfig;

  SSEChannel* channel = new SSEChannel(channelConfig, "test");
  BOOST_CHECK(channel->GetId() == "test");
}
