#pragma once

#include "common.hpp"

#include <sstream>
#include <stdexcept>

namespace dns {

struct FailedToEncodeResponse : std::runtime_error {
  using std::runtime_error::runtime_error;
};

class MessageBuilder {
public:
  MessageBuilder(size_t id);

  void AddQuestion(const std::string &domain, RecordType type, QueryClass cls);
  void AddResponse(const std::string &domain, RecordType type, QueryClass cls,
                   int ttl, const std::string &response);
  std::string BuildResponse() const;

private:

  Header header_;
  std::stringstream message_;
};

} // namespace dns
