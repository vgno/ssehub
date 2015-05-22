#include <iostream>
#include <glog/logging.h>
#include "SSEConfig.h"
#include "SSEServer.h"

#define DEFAULT_CONFIG_FILE "config.json"

using namespace std;

int main(int argc, char **argv) {
  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);

  SSEConfig conf(DEFAULT_CONFIG_FILE);
  SSEServer server(&conf);

  server.run();

  return 0;
}
