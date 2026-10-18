#pragma once

#include "common.hpp"

#include <BS_thread_pool.hpp>
#include <MinimalSocket/udp/UdpSocket.h>

#include <unordered_map>
#include <vector>

namespace dns {

class Server {
  static const int kMaxQuerySize = 1000;

public:
  Server(int port, const std::unordered_map<Question, std::vector<Answer>> &config,
         size_t threads = 10, bool ipv6 = false);
  ~Server();

  void Run();

private:
  void Process(const std::string &message, MinimalSocket::Address sender);

  std::unordered_map<Question, std::vector<Answer>> config_;
  BS::thread_pool<> thread_pool_;
  MinimalSocket::udp::Udp<true> socket_;
};

} // namespace dns
