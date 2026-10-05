#pragma once

#include <BS_thread_pool.hpp>
#include <MinimalSocket/tcp/TcpServer.h>

#include <memory>

namespace http {

class Server {
  static const int kMaxRequestSize = 16 * 1024;

public:
  Server(int port, size_t threads = 10, bool ipv6 = false);
  ~Server();

  void Run();

private:
  void Process(
      std::shared_ptr<MinimalSocket::tcp::TcpConnectionBlocking> connection);

  BS::thread_pool<> thread_pool_;
  MinimalSocket::tcp::TcpServer<true> server_;
};

} // namespace http
