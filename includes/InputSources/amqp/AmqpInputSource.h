#include <string>
#include <glog/logging.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>
#include "SSEInputSource.h"

class AmqpInputSource : public SSEInputSource {
  public:
    ~AmqpInputSource();
    void Start();

  private:
    std::string host;
    std::string user;
    std::string password;
    std::string exchange;
    std::string routingkey;
    int port;
    int heartbeatInterval;
    amqp_socket_t *amqpSocket;
    amqp_connection_state_t amqpConn;
    amqp_bytes_t amqpQueueName;

    bool Connect();
    void Disconnect();
    bool Reconnect(int delay);
    void Consume();
};
