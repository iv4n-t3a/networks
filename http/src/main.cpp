#include "server.hpp"

#include <argparse/argparse.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("http server");

  program.add_argument("--port", "-p")
      .help("TCP port for http server")
      .default_value(80)
      .scan<'i', int>()
      .nargs(1);

  program.add_argument("--threads", "-t")
      .help("Server thread pool size")
      .default_value(10)
      .scan<'i', int>()
      .nargs(1);

  program.add_argument("--ipv6", "-6")
      .help("Use ipv6 address space")
      .default_value(false)
      .implicit_value(true);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  int port = program.get<int>("--port");
  int threads = program.get<int>("--threads");
  bool ipv6 = program.get<bool>("--ipv6");

  http::Server server(port, threads, ipv6);
  server.Run();
}
