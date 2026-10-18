#include "message_parser.hpp"

#include <cstdint>
#include <optional>
#include <sstream>

namespace dns {

namespace {

uint16_t SwapBytes(uint16_t num) {
  return (num >> 8) | ((num & 0xFF) << 8);
}

} // namespace

MessageParser::MessageParser(const std::string &data) : data_(data) {}

const Header *MessageParser::GetHeader() const {
  return reinterpret_cast<const Header *>(data_.data());
}

uint16_t MessageParser::Identification() const {
  return SwapBytes(GetHeader()->identification);
}

std::optional<Question> MessageParser::GetNextQuestion() {
  if (question_ == SwapBytes(GetHeader()->questions)) {
    return std::nullopt;
  }
  question_ += 1;

  std::stringstream domain;
  domain << GetNextDomainString().value_or("");

  for (auto ds = GetNextDomainString(); ds.has_value();
       ds = GetNextDomainString()) {
    domain << '.' << ds.value();
  }

  auto type = static_cast<RecordType>(ReadUint16());
  auto cls = static_cast<QueryClass>(ReadUint16());

  return Question{
      .domain = domain.str(),
      .type = type,
      .cls = cls,
  };
}

void MessageParser::Reset() {
  pos_ = sizeof(Header);
  question_ = 0;
}

std::optional<std::string_view> MessageParser::GetNextDomainString() {
  size_t size = ReadUint8();

  if (size == 0) {
    return std::nullopt;
  }

  auto res =
      std::string_view(data_.begin() + pos_, data_.begin() + pos_ + size);
  pos_ += size;
  return res;
}

uint16_t MessageParser::ReadUint16() {
  uint16_t a = ReadUint8();
  uint16_t b = ReadUint8();
  return (a << 8) | b;
}

uint8_t MessageParser::ReadUint8() {
  uint8_t res = data_[pos_];
  pos_ += 1;
  return res;
}

} // namespace dns
