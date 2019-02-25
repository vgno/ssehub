#include <iostream>
#include <signal.h>
#include <boost/program_options.hpp>
#include "Common.h"
#include "SSEConfig.h"
#include "SSEServer.h"

#define DEFAULT_CONFIG_FILE "/etc/ssehub/config.json"

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

int main(int argc, char **argv) {
  struct sigaction sa;

  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);

  po::options_description desc("Options");
  desc.add_options()
    ("help", "produce help message")
    ("config", po::value<std::string>()->default_value(DEFAULT_CONFIG_FILE), "specify location of config file");

  po::variables_map vm = parse_options(desc, argc, argv);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  std::string conf_path = vm["config"].as<std::string>();
  SSEConfig conf;
  conf.load(conf_path.c_str());

  sa.sa_handler = shutdown;
  sa.sa_flags   = 0;

  sigemptyset(&(sa.sa_mask));
  sigaction(SIGINT, &sa, NULL);

  SSEServer server(&conf);
  server.Run();

  return 0;
}
