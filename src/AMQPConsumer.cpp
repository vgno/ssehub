#include <unistd.h>
#include "Common.h"
#include "AMQPConsumer.h"

using namespace std;

extern int stop;

/**
  Wrapper function used by ptrhead to run our Consume() function.
  @param pThis Pointer to AMQPConsumer instance object.
*/
void *AMQPConsumer::ThreadMain(void *pThis) {
  AMQPConsumer *pt = static_cast<AMQPConsumer*>(pThis);
  pt->Connect();
  pt->Consume();

  return NULL;
}

/**
  Constructor.
*/
AMQPConsumer::AMQPConsumer() {

}

/**
  Destructor.
*/
AMQPConsumer::~AMQPConsumer() {
  DLOG(INFO) << "AMQPConsumer destructor called.";

  pthread_cancel(_thread);
  Disconnect();
}

/**
  Start consume.
  @param host Hostname of amqp server.
  @param port port of amqp server.
  @param user amqp username.
  @param password amqp password.
  @param exchange amqp exchange.
  @param routingkey amqp routingkey.
  @param callback callback function.
  @param callbackArg argument for callback function.
*/
void AMQPConsumer::Start(string host, int port, string user, string password, string exchange, 
    string routingkey, void(*callback)(void*, string, string), void* callbackArg) {

  DLOG(INFO)         << "AMQPConsumer::Start():" << 
    " host: "        << host <<
    " port: "        << port <<
    " user: "        << user <<
    " password: "    << password <<
    " exchange: "    << exchange <<
    " routingkey: "  << routingkey;

  this->host = host;
  this->port = port;
  this->user = user;
  this->password = password;
  this->exchange = exchange;
  this->routingkey = routingkey;
  this->callback = callback;
  this->callbackArg = callbackArg; 

  pthread_create(&_thread, NULL, &AMQPConsumer::ThreadMain, this);
}

/**
 Disconnect from AMQP server.
*/
void AMQPConsumer::Disconnect() {
  amqp_connection_close(amqpConn, AMQP_REPLY_SUCCESS);
  amqp_destroy_connection(amqpConn);
  amqp_bytes_free(amqpQueueName);
}

/**
 Reconnect AMQP server.
 @param delay Time to wait between disconnect and connect.
*/
void AMQPConsumer::Reconnect(int delay) {
   Disconnect();
   sleep(delay);
   Connect();
}

/**
  Connect to AMQP server.
*/
bool AMQPConsumer::Connect() {
  amqp_rpc_reply_t rpc_ret;
  int ret;
  
  // Initialize connection.
  amqpConn = amqp_new_connection();
  amqpSocket = amqp_tcp_socket_new(amqpConn);
  LOG_IF(FATAL, !amqpSocket) << "Error creating amqp socket.";

  // Open connection.
  do {
    ret = amqp_socket_open(amqpSocket, host.c_str(), port);

    if (ret != AMQP_STATUS_OK) {
      LOG(ERROR) << "Failed to connect to amqp server, retrying in 5 seconds.";
      sleep(5);
    }
  } while(ret != AMQP_STATUS_OK);


  // Try to log in.
  do {
   rpc_ret = amqp_login(amqpConn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
    user.c_str(), password.c_str());

    if (rpc_ret.reply_type != AMQP_RESPONSE_NORMAL) {
      LOG(ERROR) << "Failed to log into AMQP, retrying in 5 seconds.";
      sleep(5);
    }
  } while(rpc_ret.reply_type != AMQP_RESPONSE_NORMAL);

  // Connect to channel.
  amqp_channel_open(amqpConn, 1);
  rpc_ret = amqp_get_rpc_reply(amqpConn);

  if (rpc_ret.reply_type != AMQP_RESPONSE_NORMAL) {
    LOG(ERROR) << "Failed to open AMQP channel, trying to reconnect in 5 seconds.";
    Reconnect(5);
  }

  // Declare queue.
  amqp_queue_declare_ok_t *r = amqp_queue_declare(amqpConn, 1, amqp_empty_bytes, 0, 0, 0, 1,
                                 amqp_empty_table);

  if (amqp_get_rpc_reply(amqpConn).reply_type != AMQP_RESPONSE_NORMAL) {
    LOG(ERROR) << "Failed to declare queue, trying to reconnect in 5 seconds.";
    Reconnect(5);
  }

  amqpQueueName = amqp_bytes_malloc_dup(r->queue);
  LOG_IF(FATAL, amqpQueueName.bytes == NULL) << "Out memory while copying queue name";

  // Bind queue.
  amqp_queue_bind(amqpConn, 1, amqpQueueName, amqp_cstring_bytes(exchange.c_str()), 
  amqp_cstring_bytes(routingkey.c_str()), amqp_empty_table);

  rpc_ret = amqp_get_rpc_reply(amqpConn);

  if (rpc_ret.reply_type != AMQP_RESPONSE_NORMAL) {
    LOG(ERROR) << "Failed to bind to AMQP queue, trying to reconnect in 5 seconds.";
    Reconnect(5);
  }

  LOG(INFO) << "Connected to AMQP server " << host << ":" << port << ".";

  return true;
}

/**
  Start consumption from AMQP server.
*/
void AMQPConsumer::Consume() {
  while(!stop) {
    amqp_envelope_t envelope;
    amqp_rpc_reply_t ret;

    amqp_basic_consume(amqpConn, 1, amqpQueueName, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    ret = amqp_get_rpc_reply(amqpConn);

    // Free up memory pool.
    amqp_maybe_release_buffers(amqpConn);

    if (ret.reply_type != AMQP_RESPONSE_NORMAL) {
      LOG(ERROR) << "Failed to consume AMQP queue, trying to reconnect in 5 seconds.";
      Reconnect(5);
      continue;
    }

    ret = amqp_consume_message(amqpConn, &envelope, NULL, 0);

    if (ret.reply_type != AMQP_RESPONSE_NORMAL) {
      LOG(ERROR) << "Error consuming message.";
    } else {
     string msg;
     string key;
     msg.insert(0, (const char*)envelope.message.body.bytes, envelope.message.body.len);
     key.insert(0, (const char*)envelope.routing_key.bytes, envelope.routing_key.len);
     callback(callbackArg, key, msg);
    }

    amqp_destroy_envelope(&envelope);
  }
}
