#include "response_builder.hpp"

#include "common.hpp"

#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>
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

void WriteIp(std::stringstream &ss, const std::string &ip) {
  std::stringstream ipstream(ip);
  std::string octet;
  while (std::getline(ipstream, octet, '.')) {
    WriteUint8(ss, static_cast<uint8_t>(std::stoi(octet)));
  }
}

} // namespace

ResponseBuilder::ResponseBuilder(size_t id) {
  std::memset(&header_, 0, sizeof(header_));

  header_.identification = id;
  header_.flags.QR = 1;
  header_.flags.opcode = Header::Opcode::STANDART;
  header_.flags.rcode = Header::RCode::NO_ERROR;
}

void ResponseBuilder::AddQuestion(const std::string &domain, RecordType type,
                                  QueryClass cls) {
  header_.questions += 1;
  WriteDomain(message_, domain);
  WriteUint16(message_, static_cast<uint16_t>(type));
  WriteUint16(message_, static_cast<uint16_t>(cls));
}

void ResponseBuilder::AddResponse(const std::string &domain, RecordType type,
                                  QueryClass cls, int ttl,
                                  const std::string &response) {
  header_.answers += 1;
  WriteDomain(message_, domain);
  WriteUint16(message_, static_cast<uint16_t>(type));
  WriteUint16(message_, static_cast<uint16_t>(cls));
  WriteUint32(message_, static_cast<uint32_t>(ttl));

  if (type == RecordType::A) {
    WriteUint16(message_, 4);
    WriteIp(message_, response);
  } else if (type == RecordType::AAAA) {
    throw std::runtime_error("AAAA records are not implemented now");
  } else {
    WriteUint16(message_, static_cast<uint16_t>(response.size()));
    message_ << response;
  }
}

std::string ResponseBuilder::BuildResponse() const {
  std::stringstream res;

  WriteHeader(res, header_);
  res << message_.str();

  return res.str();
}

} // namespace dns
