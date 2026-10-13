#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "common.hpp"

namespace dns {

class RequestParser {
public:
  RequestParser(const std::string &data);

  uint16_t Identification() const;

  std::optional<Question> GetNextQuestion();

  void Reset();

private:
  const Header *GetHeader() const;
  std::optional<std::string_view> GetNextDomainString();

  uint16_t ReadUint16();
  uint8_t ReadUint8();

  const std::string &data_;
  size_t pos_ = sizeof(Header);
  size_t question_ = 0;
};

} // namespace dns
