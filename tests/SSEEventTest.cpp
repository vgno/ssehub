#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SSEEventTest
#include <boost/test/unit_test.hpp>

#include "SSEEvent.cpp"

BOOST_AUTO_TEST_CASE(canConstructSSEChannel) {
  std::string msg = "{ \"id\": 1, \"path\": \"/test\", \"event\": \"message\", \"data\": \"Test message\" }";

  SSEEvent* event = new SSEEvent(msg);
  BOOST_CHECK(event->compile() == true);
  BOOST_CHECK(event->getid() == "1");
  BOOST_CHECK(event->getpath() == "/test");

  event->setpath("/newPath");
  BOOST_CHECK(event->getpath() == "/newPath");

}
