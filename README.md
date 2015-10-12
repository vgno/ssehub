SSEHub (Server-Sent Events streaming server)
============================================

Performant SSE-server implementation written in C++ that supports multiple channels and high traffic loads.

# Key features

  - Supports multiple channels both staticly and dynamically-on-the-fly configured.
  - Configurable history cache that can be requested by clients upon reconnect using lastEventId.
  - Configurable "keep-alive" pings.
  - CORS support.
  - RabbitMQ as input source.
  - Polyfill client support.
  - Statistics endpoint (/stats).

# Building and running

Install dependencies (using apt in this example):
```
apt-get install g++ make libgoogle-glog-dev libboost-dev libleveldb-dev \
libboost-system-dev libboost-thread-dev librabbitmq-dev
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
./ssehub --config path/to/config.json (will use ./conf/config.json as default).
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
    "enabled": "false",
    "host": "127.0.0.1",
    "port": 5672,
    "user": "guest",
    "password": "guest",
    "exchange": "amq.fanout"
  },
  "redis": {
    "host": "127.0.0.1",
    "port": 6379,
    "numConnections": 5
  },
  "leveldb": {
    "storageDir": "/tmp"
  },
  "default": {
    "cacheAdapter": "leveldb",
    "cacheLength": 500,
    "allowedOrigins":  "*",
    "restrictPublish": [
      "127.0.0.1"
    ]
  },
  "channels": [
    {
        "path": "test",
        "allowedOrigins": ["https://some.host"],
        "cacheLength": 5
    },
    {
        "path": "test2",
        "cacheAdapter": "memory",
        "restrictPublish": [
         "10.0.0.0/8"
        ]
    }
  ]
}
```

# Event format

Currently we support POST and RabbitMQ fanout queue as input source.
For POST you can restrict access on ip/subnet basis with the restrictPublish configuration directive.
You can do this both globally in the default section of the config or per channel basis.

Events should be sent in the following format:

```json
{
    "id": 1,
    "path": "test",
    "event": "message",
    "data": "My event data."
}
```

When using POST for publishing events the "path" element in the event is ignored and replaced with the channel/endpoint you are posting to.

# Example using POST

Publish event to channel **test** with curl:

```
curl -v -X POST http://127.0.0.1:8080/test \
-d '{ "id": 1, "event": "message", "data": "Test message" }'
```

# Dynamic creation of channels
If ```allowUndefinedChannels``` is set to true in the config the channel will be created when the first event is sent to the channel.

# License

SSEHub is licensed under MIT.
See LICENSE.
