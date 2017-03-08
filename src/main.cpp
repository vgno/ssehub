#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include "Common.h"
#include "SSEConfig.h"
#include "SSEServer.h"

#define DEFAULT_CONFIG_FILE "./conf/config.json"

using namespace std;
namespace po = boost::program_options;

int stop = 0;

void shutdown(int sigid) {
  LOG(INFO) << "Exiting.";
  stop = 1;
}

po::variables_map parse_options(po::options_description desc, int argc, char **argv) {
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  po::notify(vm);

  return vm;
}

void StartServer(const string& conf_path) {
  SSEConfig conf;
  conf.load(conf_path.c_str());
  SSEServer server(&conf);
  server.Run();
  exit(0);
}

int main(int argc, char **argv) {
  struct sigaction sa;
  vector<int> worker_pids;

  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);

  po::options_description desc("Options");
  desc.add_options()
    ("help", "produce help message")
    ("config", po::value<std::string>()->default_value(DEFAULT_CONFIG_FILE), "specify location of config file");

  po::variables_map vm = parse_options(desc, argc, argv);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  std::string conf_path = vm["config"].as<std::string>();

  sa.sa_handler = shutdown;
  sa.sa_flags   = 0;

  sigemptyset(&(sa.sa_mask));
  sigaction(SIGINT, &sa, NULL);

  long nCPUS = sysconf(_SC_NPROCESSORS_ONLN);
  (nCPUS > 0) || (nCPUS = 1);

  if (nCPUS == 1) {
   StartServer(conf_path);
  }

  LOG(INFO) << "Starting " << nCPUS << " workers.";
  for (int i = 0; i < nCPUS; i++) {
    int _pid = fork();

    if (_pid < 0) {
      LOG(ERROR) << "Could not fork fork() worker " << i;
      abort();
    } else if (_pid == 0) {
      StartServer(conf_path);
    }

    LOG(INFO) << "Started worker with PID: " << _pid;
    worker_pids.push_back(_pid);
  }

  BOOST_FOREACH(int worker_pid, worker_pids) {
    int status;
    waitpid(worker_pid, &status, 0);
    LOG(INFO) << "Worker with pid " << worker_pid << " exited.";
  }

  return 0;
}
