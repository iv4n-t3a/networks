#include "response_builder.hpp"

#include "common.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

namespace dns {

ResponseBuilder::ResponseBuilder(size_t id) {
  header_.identification = id;
  header_.flags = Header::Flags{.QR = 1,
                                .opcode = Header::Opcode::STANDART,
                                .AA = 0,
                                .TC = 0,
                                .RD = 0,
                                .RA = 0,
                                .zero = 0,
                                .rcode = Header::RCode::NO_ERROR};
  header_.answers = 0;
  header_.questions = 0;
  header_.additional_RR = 0;
  header_.authority_RR = 0;
}

void ResponseBuilder::AddQuestion(const std::string &domain, RecordType type,
                                  QueryClass cls) {
  header_.questions += 1;
  WriteDomain(domain);
  WriteUint16(static_cast<uint16_t>(type));
  WriteUint16(static_cast<uint16_t>(cls));
}

void ResponseBuilder::AddResponse(const std::string &domain, RecordType type,
                                  QueryClass cls, int ttl,
                                  const std::string &rdata) {
  header_.answers += 1;
  WriteDomain(domain);
  WriteUint16(static_cast<uint16_t>(type));
  WriteUint16(static_cast<uint16_t>(cls));
  WriteUint32(static_cast<uint32_t>(ttl));
  WriteUint16(static_cast<uint16_t>(rdata.size()));
  response_ << rdata;
}

void ResponseBuilder::WriteDomain(const std::string &domain) {
  std::stringstream dstream(domain);

  for (std::string subdomain; std::getline(dstream, subdomain, '.');) {
    WriteUint8(subdomain.size());
    response_ << subdomain;
  }
  WriteUint8(0);
}

void ResponseBuilder::WriteUint32(uint32_t num) {
  WriteUint8((num >> 24) & 0xFF);
  WriteUint8((num >> 16) & 0xFF);
  WriteUint8((num >> 8) & 0xFF);
  WriteUint8(num & 0xFF);
}
void ResponseBuilder::WriteUint16(uint16_t num) {
  WriteUint8(num >> 8);
  WriteUint8(num & 0xFF);
}
void ResponseBuilder::WriteUint8(uint8_t num) {
  response_.write(reinterpret_cast<char *>(&num), 1);
}

std::string ResponseBuilder::BuildResponse() const {
  std::stringstream res;
  res.write(reinterpret_cast<const char *>(&header_), sizeof(Header));
  res << response_.str();
  return res.str();
}

} // namespace dns
