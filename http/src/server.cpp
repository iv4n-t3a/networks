#include "server.hpp"

#include <iostream>
#include <tuple>

namespace http {

Server::Server(int port, size_t threads, bool ipv6)
    : server_(port, ipv6 ? MinimalSocket::AddressFamily::IP_V6
                         : MinimalSocket::AddressFamily::IP_V4),
      thread_pool_(threads) {}

Server::~Server() { server_.shutDown(); }

void Server::Run() {
  server_.open();

  for (;;) {
    auto connection = server_.acceptNewClient();

    auto shared_connection =
        std::make_shared<MinimalSocket::tcp::TcpConnectionBlocking>(
            std::move(connection));

    std::ignore = thread_pool_.submit_task([this, shared_connection]() {
      Process(shared_connection);
    });
  }
}

void Server::Process(
    std::shared_ptr<MinimalSocket::tcp::TcpConnectionBlocking> connection) {
  std::cout << "Got http request" << std::endl;

  std::ignore = connection->receive(kMaxRequestSize);

  std::cout << "finished processing http request" << std::endl;
}

} // namespace http
