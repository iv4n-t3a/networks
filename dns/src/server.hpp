#pragma once

#include <BS_thread_pool.hpp>
#include <MinimalSocket/udp/UdpSocket.h>

namespace dns {

class Server {
  static const int kMaxQuerySize = 1000;

public:
  Server(int port, size_t threads = 10, bool ipv6 = false);
  ~Server();

  void Run();

private:
  void Process(const std::string &message, MinimalSocket::Address sender);

  BS::thread_pool<> thread_pool_;
  MinimalSocket::udp::Udp<true> socket_;
};

} // namespace dns
