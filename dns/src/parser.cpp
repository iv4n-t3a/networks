#include "parser.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>

namespace dns {

namespace {

uint16_t ChangeEndianess(uint16_t num) {
  return (num >> 8) | ((num & 0xFF) << 8);
}

} // namespace

Parser::Parser(const std::string &data) : data_(data) {}

const Header *Parser::GetHeader() const {
  return reinterpret_cast<const Header *>(data_.data());
}

std::optional<Question> Parser::GetNextQuestion() {
  if (question_ == ChangeEndianess(GetHeader()->questions)) {
    return std::nullopt;
  }
  question_ += 1;

  std::stringstream domain;
  domain << "www";

  auto ds = GetNextDomainString();

  while (ds.has_value()) {
    domain << '.' << ds.value();
    ds = GetNextDomainString();
  }

  auto type = static_cast<RecordType>(GetNextUint16());
  auto cls = static_cast<QueryClass>(GetNextUint16());

  return Question{
      .domain = domain.str(),
      .type = type,
      .cls = cls,
  };
}

void Parser::Reset() {
  pos_ = sizeof(Header);
  question_ = 0;
}

std::optional<std::string_view> Parser::GetNextDomainString() {
  size_t size = GetNextUint8();

  if (size == 0) {
    return std::nullopt;
  }

  auto res =
      std::string_view(data_.begin() + pos_, data_.begin() + pos_ + size);
  pos_ += size;
  return res;
}

uint16_t Parser::GetNextUint16() {
  uint16_t a = GetNextUint8();
  uint16_t b = GetNextUint8();
  return a | (b << 16);
}

uint8_t Parser::GetNextUint8() {
  uint8_t res = data_[pos_];
  pos_ += 1;
  return res;
}

} // namespace dns
