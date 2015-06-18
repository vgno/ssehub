#include <iostream>
#include <signal.h>
#include "Common.h"
#include "SSEConfig.h"
#include "SSEServer.h"

#define DEFAULT_CONFIG_FILE "./conf/config.json"

using namespace std;

int stop = 0;

void shutdown(int sigid) {
  LOG(INFO) << "Exiting.";
  stop = 1;
}

int main(int argc, char **argv) {
  struct sigaction sa;

  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);

  SSEConfig conf;
  if (!conf.load(DEFAULT_CONFIG_FILE)) {
    LOG(ERROR) << "Failed to parse config file.";
    return 1;
  }

  sa.sa_handler = shutdown;
  sa.sa_flags   = 0;

  sigemptyset(&(sa.sa_mask));
  sigaction(SIGINT, &sa, NULL);

  SSEServer server(&conf);
  server.Run();

  return 0;
}
