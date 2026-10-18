#include "message_builder.hpp"

#include "common.hpp"

#include <cstdint>
#include <cstring>
#include <format>
#include <sstream>
#include <string>

namespace dns {

namespace {

void WriteUint8(std::stringstream &ss, uint8_t num) {
  ss.write(reinterpret_cast<char *>(&num), 1);
}

void WriteUint16(std::stringstream &ss, uint16_t num) {
  WriteUint8(ss, num >> 8);
  WriteUint8(ss, num & 0xFF);
}

void WriteUint32(std::stringstream &ss, uint32_t num) {
  WriteUint8(ss, (num >> 24) & 0xFF);
  WriteUint8(ss, (num >> 16) & 0xFF);
  WriteUint8(ss, (num >> 8) & 0xFF);
  WriteUint8(ss, num & 0xFF);
}

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

void WriteIpv4(std::stringstream &ss, const std::string &ip) try {
  std::stringstream ipstream(ip);
  std::string octet;
  size_t octets_count = 0;

  while (std::getline(ipstream, octet, '.')) {
    int32_t val = std::stoi(octet);

    if (val < 0 || val >= std::numeric_limits<uint8_t>::max()) {
      throw FailedToEncodeResponse(
          std::format("Invalid ipv4 {}: octet {} is too big", ip, octet));
    }

    WriteUint8(ss, static_cast<uint8_t>(val));
    octets_count += 1;
  }

  if (octets_count != 4) {
    throw FailedToEncodeResponse(
        std::format("Invalid ipv4 {}: Ipv4 must have 4 octets", ip));
  }

} catch (const std::exception &ex) {
  throw FailedToEncodeResponse(
      std::format("Invalid ipv4 {}: {}", ip, ex.what()));
}

void WriteIpv6(std::stringstream &ss, const std::string &ip) try {
  std::stringstream ipstream(ip);
  std::string hextet;
  size_t hextet_count = 0;

  while (std::getline(ipstream, hextet, ':')) {
    int32_t val = std::stoi(hextet, 0, 16);

    if (val < 0 || val >= std::numeric_limits<uint16_t>::max()) {
      throw FailedToEncodeResponse(
          std::format("Invalid ipv6 {}: hextet {} is too big", ip, hextet));
    }

    WriteUint16(ss, static_cast<uint16_t>(val));
    hextet_count += 1;
  }

  if (hextet_count != 8) {
    throw FailedToEncodeResponse(
        std::format("Invalid ipv6 {}: Ipv6 must have 8 hextets", ip));
  }
} catch (const std::exception &ex) {
  throw FailedToEncodeResponse(
      std::format("Invalid ipv6 {}: {}", ip, ex.what()));
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
    WriteIpv4(message_, response);
    break;
  }
  case RecordType::AAAA: {
    const int kIpv6AddressSize = 16;
    WriteUint16(message_, kIpv6AddressSize);
    WriteIpv6(message_, response);
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
