#ifndef SSEDATASOURCE_H
#define SSEDATASOURCE_H

#include <string>
#include <boost/function.hpp>
#include <boost/thread.hpp>

// Forward declarations.
class SSEServer;
class SSEConfig;

typedef boost::function<void(std::string)> DataSourceCallback;

class SSEInputSource {
  public:
    SSEInputSource();
    virtual ~SSEInputSource();
    void Init(SSEServer* server, DataSourceCallback callback);
    void Run();
    virtual void Start() {};
    void KillThread();

  protected:
    SSEServer* _server;
    SSEConfig* _config;
    boost::thread _thread;
    DataSourceCallback _callback;
};

#endif
