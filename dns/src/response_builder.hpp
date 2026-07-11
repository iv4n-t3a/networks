#pragma once

#include "common.hpp"

#include <cstdint>
#include <sstream>

namespace dns {

class ResponseBuilder {
public:
  ResponseBuilder(size_t id);

  void AddQuestion(const std::string &domain, RecordType type, QueryClass cls);
  void AddResponse(const std::string &domain, RecordType type, QueryClass cls,
                   int ttl, const std::string &rdata);
  std::string BuildResponse() const;

private:
  void WriteDomain(const std::string &domain);
  void WriteUint32(uint32_t);
  void WriteUint16(uint16_t);
  void WriteUint8(uint8_t);

  Header header_;
  std::stringstream response_;
};

} // namespace dns
