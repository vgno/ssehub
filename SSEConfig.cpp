#include "SSEConfig.h" 
#include "lib/cJSON/cJSON.h"
#include <glog/logging.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>

using namespace std;

bool SSEConfig::load(const char *file) {
  ifstream ifs(file);
  cJSON *json;

  if (ifs.is_open()) {
    stringstream ss;
    ss << ifs.rdbuf();
    ifs.close();

    json = cJSON_Parse(ss.str().c_str());

    cJSON *server = cJSON_GetObjectItem(json, "server");
    cJSON *amqp = cJSON_GetObjectItem(json, "amqp");

    serverConfig.port              = cJSON_GetObjectItem(server, "port")->valueint;
    serverConfig.bindIP            = cJSON_GetObjectItem(server, "bind-ip")->valuestring;
    serverConfig.logDir            = cJSON_GetObjectItem(server, "logDir")->valuestring;
    serverConfig.threadsPerChannel = cJSON_GetObjectItem(server, "threadsPerChannel")->valueint;
    serverConfig.pingInterval      = cJSON_GetObjectItem(server, "pingInterval")->valueint;

    amqpConfig.host         = cJSON_GetObjectItem(amqp, "host")->valuestring;
    amqpConfig.port         = cJSON_GetObjectItem(amqp, "port")->valueint;
    amqpConfig.user         = cJSON_GetObjectItem(amqp, "user")->valuestring;
    amqpConfig.password     = cJSON_GetObjectItem(amqp, "password")->valuestring;
    amqpConfig.exchange     = cJSON_GetObjectItem(amqp, "exchange")->valuestring;

    cJSON_Delete(json);
  } else {
    LOG(FATAL) << "Error opening configfile " << file;
    exit(1);
  }

  return true;
}

SSEServerConfig& SSEConfig::getServer() {
  return serverConfig;
}

SSEAmqpConfig& SSEConfig::getAmqp() {
  return amqpConfig;
}

SSEConfig::SSEConfig(string file) {
  load(file.c_str());
}
