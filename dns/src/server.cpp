#include "server.hpp"

#include "common.hpp"
#include "config.hpp"
#include "message_parser.hpp"
#include "message_builder.hpp"

#include <iostream>

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
                     MinimalSocket::Address sender) try {
  std::cout << "Got dns query" << std::endl;

  MessageParser parser(message);
  MessageBuilder builder(parser.Identification());
  auto q_opt = parser.GetNextQuestion();

  while (q_opt.has_value()) {
    auto q = q_opt.value();
    builder.AddQuestion(q.domain, q.type, q.cls);
    std::cout << "Query contains question: " << q.domain << std::endl;
    q_opt = parser.GetNextQuestion();
  }

  parser.Reset();
  q_opt = parser.GetNextQuestion();

  while (q_opt.has_value()) {
    auto q = q_opt.value();

    for (const auto [response_q, response_a] : kDnsConfig) {
      if (q == response_q) {
        builder.AddResponse(q.domain, q.type, q.cls, response_a.ttl,
                            response_a.response);
      }
    }
    q_opt = parser.GetNextQuestion();
  }

  std::string response = builder.BuildResponse();

  socket_.sendTo(response, sender);

  std::cout << "Finished processing dns query" << std::endl;
} catch (const std::exception& ex) {
  std::cout << "Failed to process dns query: " << ex.what() << std::endl;

  // TODO: propper error handling
  std::cout << "Error handling is not implemented yet, ignoring mailformed query" << std::endl;
}

} // namespace dns
