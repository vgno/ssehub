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
    cJSON *channels = cJSON_GetObjectItem(json, "channels");

    serverConfig.port   = cJSON_GetObjectItem(server, "port")->valueint;
    serverConfig.bindIP = cJSON_GetObjectItem(server, "bind-ip")->valuestring;
    serverConfig.logDir = cJSON_GetObjectItem(server, "logDir")->valuestring;

    for (int i = 0; i < cJSON_GetArraySize(channels); i++) {
      cJSON *item = cJSON_GetArrayItem(channels, i);

      SSEChannelConfig chanItem;
      chanItem.ID = item->string;
      chanItem.amqpQueue    = cJSON_GetObjectItem(item, "queueName")->valuestring;
      chanItem.amqpUser     = cJSON_GetObjectItem(item, "user")->valuestring;
      chanItem.amqpPassword = cJSON_GetObjectItem(item, "password")->valuestring;
      chanItem.pingInterval = cJSON_GetObjectItem(item, "pingInterval")->valueint;
    
      /* For now just use the default amqp host and port.
         Later we might support multiple amqp's.  */
      chanItem.amqpHost = cJSON_GetObjectItem(amqp, "host")->valuestring;
      chanItem.amqpPort = cJSON_GetObjectItem(amqp, "port")->valueint;

      chanConfList.push_back(chanItem);
    }

    cJSON_Delete(json);
  } else {
    LOG(FATAL) << "Error opening configfile " << file;
    exit(1);
  }

  return true;
}

SSEChannelConfigList SSEConfig::getChannels() {
  return chanConfList;
}

SSEServerConfig& SSEConfig::getServer() {
  return serverConfig;
}

SSEConfig::SSEConfig(string file) {
  load(file.c_str());
}