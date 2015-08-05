SSEHUB (Server-Sent Events streaming server)
============================================

Performant SSE-server implementation written in C++ that supports multiple channels and high traffic loads.
# Key features
  - Supports multiple channels both staticly and dynamically-on-the-fly configured.
  - Configureable history cache that can be requested by clients upon reconnect using lastEventId.
  - Configureable "keep-alive" pings.
  - CORS support.
  - RabbitMQ as input source.
  - Polyfill client support.
  - Statistics endpoint (/stats).

# Building and running
Install dependencies (using apt in this example):
```
apt-get install g++ make libgoogle-glog-dev libboost-dev libboost-system-dev libboost-thread-dev librabbitmq-dev
```

Checkout sourcecode:
```
git clone git@github.com:vgno/ssehub.git
```

Compile:
```
cd ssehub && make
```

Run:
```
./ssehub -config path/to/config.json (will use ./conf/config.json as default).
```

There is also a Dockerfile you can use to build a docker image.

# Example configuration
```json
{
  "server": {
    "port": 8080,
    "bind-ip": "0.0.0.0",
    "logdir": "./",
    "pingInterval": 5,
    "threadsPerChannel": 2,
    "allowUndefinedChannels": "true"
  },
  "amqp": {
    "host": "127.0.0.1",
    "port": 5672,
    "user": "guest",
    "password": "guest",
    "exchange": "amq.fanout"
  },
  "default": {
    "historyLength": 500,
    "allowedOrigins":  "*"
  },
  "channels": [
    {
        "path": "test",
        "allowedOrigins": ["https://some.host"],
        "historyLength": 50
    },
    {
        "path": "test2"
    }
  ]
}
```
# Event format
Currently we support RabbitMQ fanout queue as input source.
Events should be sent in the following format:

```json
{
    "path": "test",
    "id": 1,
    "data": "My event data."
}
```
