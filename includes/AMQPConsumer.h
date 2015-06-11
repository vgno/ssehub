#ifndef AMQPCONSUMER_H
#define AMQPCONSUMER_H

#include <string>
#include <glog/logging.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>
#include <pthread.h>

using namespace std;

class AMQPConsumer {
  public:
    AMQPConsumer();
    ~AMQPConsumer();

    void Start(string host, int port, string user, string password, string exchange, string routingkey, void(*callback)(void*, string, string), void* callbackArg);
    bool Connect();
    void Disconnect();
    void Reconnect(int delay);
    void Consume();
    static void *ThreadMain(void*);

  private:
    pthread_t _thread;
    string host;
    string user;
    string password;
    string exchange;
    string routingkey;
    int port;
    void *callbackArg;
    amqp_socket_t *amqpSocket;
    amqp_connection_state_t amqpConn;
    amqp_bytes_t amqpQueueName;

    void(*callback)(void*, string, string);
};

#endif
