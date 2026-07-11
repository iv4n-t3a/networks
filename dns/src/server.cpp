#include "server.hpp"

#include "common.hpp"
#include "parser.hpp"
#include "response_builder.hpp"

#include <MinimalSocket/core/Address.h>

namespace dns {

Server::Server(int port, size_t threads, bool ipv6)
    : socket_(port, ipv6 ? MinimalSocket::AddressFamily::IP_V6
                         : MinimalSocket::AddressFamily::IP_V4),
      thread_pool_(threads) {}

Server::~Server() { socket_.shutDown(); }

void Server::Run() {
  socket_.open();

  for (;;) {
    auto message = socket_.receive(kMaxQuerySize);

    if (!message.has_value()) {
      continue;
    }

    const std::string &content = message.value().received_message;
    const auto &sender = message.value().sender;

    std::ignore = thread_pool_.submit_task(
        [this, content, sender]() { Process(content, sender); });
  }
}

void Server::Process(const std::string &message,
                     MinimalSocket::Address sender) {
  std::cout << "Got dns query" << std::endl;

  Parser parser(message);
  ResponseBuilder builder(parser.Identication());
  auto q_opt = parser.GetNextQuestion();

  while (q_opt.has_value()) {
    auto q = q_opt.value();
    builder.AddQuestion(q.domain, q.type, q.cls);
    q_opt = parser.GetNextQuestion();
  }

  parser.Reset();
  q_opt = parser.GetNextQuestion();

  while (q_opt.has_value()) {
    auto q = q_opt.value();
    builder.AddResponse(q.domain, q.type, q.cls, 0, "127.0.0.1");
    q_opt = parser.GetNextQuestion();
  }

  std::string response = builder.BuildResponse();

  socket_.sendTo(response, sender);

  std::cout << "finished processing dns query" << std::endl;
}

} // namespace dns
