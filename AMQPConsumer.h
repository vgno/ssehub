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

    void start(string host, int port, string user, string password, string routingkey, void(*callback)(void*, string), void* callbackArg);
    bool connect();
    void consume();
    static void *threadMain(void*);

  private:
    string host;
    string user;
    string password;
    string routingkey;
    int port;
    pthread_t thread;
    void(*callback)(void*, string);
    void *callbackArg;
    amqp_socket_t *amqpSocket = NULL;
    amqp_connection_state_t amqpConn;
    amqp_bytes_t amqpQueueName;
};

#endif