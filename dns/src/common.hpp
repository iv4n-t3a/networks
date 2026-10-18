#pragma once

#include "macros.hpp"

#include <cstdint>
#include <format>
#include <functional>
#include <string>

namespace dns {

struct PACKED Header {
  enum class Opcode {
    STANDART = 0,
    INVERSE = 1,
    STATUS = 2,
  };

  enum class RCode {
    NO_ERROR = 0,
    INVALID_FORMAT = 1,
    SERVER_FAILURE = 2,
    NAME_ERROR = 3,
    UNSUPPORTED_OPERATION = 4,
    POLICY = 5,
  };

  struct PACKED Flags {
    RCode rcode : 4;
    int zero : 3;
    bool RA : 1;
    bool RD : 1;
    bool TC : 1;
    bool AA : 1;
    Opcode opcode : 4;
    bool QR : 1;
  };

  uint16_t identification;
  Flags flags;
  uint16_t questions;
  uint16_t answers;
  uint16_t authority_RR;
  uint16_t additional_RR;
};

static_assert(sizeof(Header::Flags) == 2);
static_assert(sizeof(Header) == 12);

enum class RecordType {
  A = 1,
  AAAA = 28,
};

enum class QueryClass {
  IN = 1,
};

struct Question {
  std::string domain;
  RecordType type;
  QueryClass cls;
};

struct Answer {
  std::string response;
  int ttl;
};

inline bool operator==(const Question &q1, const Question &q2) {
  return q1.domain == q2.domain && q1.type == q2.type && q1.cls == q2.cls;
}

} // namespace dns

namespace std {
template <> struct hash<dns::Question> {
  inline size_t operator()(const dns::Question &q) const noexcept {
    return hash<std::string>()(std::format("{}|{}|{}", static_cast<int>(q.cls),
                                           static_cast<int>(q.type), q.domain));
  }
};
} // namespace std
