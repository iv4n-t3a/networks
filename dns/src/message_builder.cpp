#include "message_builder.hpp"

#include "common.hpp"
#include "io.hpp"

#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

namespace dns {

namespace {

void WriteDomain(std::stringstream &ss, const std::string &domain) {
  std::stringstream dstream(domain);

  for (std::string subdomain; std::getline(dstream, subdomain, '.');) {
    WriteUint8(ss, subdomain.size());
    ss << subdomain;
  }
  WriteUint8(ss, 0);
}

void WriteHeader(std::stringstream &ss, const Header &header) {
  WriteUint16(ss, header.identification);
  WriteUint16(ss, *reinterpret_cast<const uint16_t *>(&header.flags));
  WriteUint16(ss, header.questions);
  WriteUint16(ss, header.answers);
  WriteUint16(ss, header.authority_RR);
  WriteUint16(ss, header.additional_RR);
}

} // namespace

MessageBuilder::MessageBuilder(size_t id) {
  std::memset(&header_, 0, sizeof(header_));

  header_.identification = id;
  header_.flags.QR = 1;
  header_.flags.opcode = Header::Opcode::STANDART;
  header_.flags.rcode = Header::RCode::NO_ERROR;
}

void MessageBuilder::AddQuestion(const std::string &domain, RecordType type,
                                 QueryClass cls) {
  header_.questions += 1;
  WriteDomain(message_, domain);
  WriteUint16(message_, static_cast<uint16_t>(type));
  WriteUint16(message_, static_cast<uint16_t>(cls));
}

void MessageBuilder::AddResponse(const std::string &domain, RecordType type,
                                 QueryClass cls, int ttl,
                                 const std::string &response) {
  header_.answers += 1;
  WriteDomain(message_, domain);
  WriteUint16(message_, static_cast<uint16_t>(type));
  WriteUint16(message_, static_cast<uint16_t>(cls));
  WriteUint32(message_, static_cast<uint32_t>(ttl));

  switch (type) {
  case RecordType::A: {
    const int kIpv4AddressSize = 4;
    WriteUint16(message_, kIpv4AddressSize);
    message_ << response;
    break;
  }
  case RecordType::AAAA: {
    const int kIpv6AddressSize = 16;
    WriteUint16(message_, kIpv6AddressSize);
    message_ << response;
    break;
  }
  }
}

std::string MessageBuilder::BuildResponse() const {
  std::stringstream res;

  WriteHeader(res, header_);
  res << message_.str();

  return res.str();
}

} // namespace dns
