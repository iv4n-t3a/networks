#pragma once

#include "macros.hpp"

#include <cstdint>
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
    bool QR : 1;
    Opcode opcode : 4;
    bool AA : 1;
    bool TC : 1;
    bool RD : 1;
    bool RA : 1;
    int zero : 3;
    RCode rcode : 4;
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

} // namespace dns
