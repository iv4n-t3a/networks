#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "common.hpp"

namespace dns {

class Parser {
public:
  Parser(const std::string &data);

  uint16_t Identication() const { return GetHeader()->identification; }

  std::optional<Question> GetNextQuestion();

  void Reset();

private:
  const Header *GetHeader() const;
  std::optional<std::string_view> GetNextDomainString();

  uint16_t GetNextUint16();
  uint8_t GetNextUint8();

  const std::string &data_;
  size_t pos_ = sizeof(Header);
  size_t question_ = 0;
};

} // namespace dns
