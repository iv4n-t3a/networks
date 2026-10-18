#include "config.hpp"

#include "common.hpp"
#include "io.hpp"

#include <cerrno>
#include <cstring>
#include <format>
#include <fstream>

#include <nlohmann/json.hpp>

namespace dns {

namespace {

const auto kRecordTypeField = std::string{"type"};
const auto kRecordResponseField = std::string{"response"};
const int kDefaultTtl = 60;

RecordType ParseRecordType(const std::string &record_type) {
  if (record_type == "A") {
    return RecordType::A;
  } else if (record_type == "AAAA") {
    return RecordType::AAAA;
  } else {
    throw ParseError(
        std::format("Unknown record type {}. Known record types are: A, AAAA",
                    record_type));
  }
}

std::string ParseResponseA(const std::string &ip) try {
  std::stringstream res;
  std::stringstream ipstream(ip);
  std::string octet;
  size_t octets_count = 0;

  while (std::getline(ipstream, octet, '.')) {
    int32_t val = std::stoi(octet);

    if (val < 0 || val >= std::numeric_limits<uint8_t>::max()) {
      throw ParseError(
          std::format("Invalid ipv4 {}: octet {} is too big", ip, octet));
    }

    WriteUint8(res, static_cast<uint8_t>(val));
    octets_count += 1;
  }

  if (octets_count != 4) {
    throw ParseError(
        std::format("Invalid ipv4 {}: Ipv4 must have 4 octets", ip));
  }

  return res.str();
} catch (const std::exception &ex) {
  throw ParseError(std::format("Invalid ipv4 {}: {}", ip, ex.what()));
}

std::string ParseResponseAAAA(const std::string &ip) try {
  if (ip.find("::") != std::string::npos) {
    throw ParseError(std::format("Invalid ipv6 {}: :: shortening is not supported", ip));
  }

  std::stringstream res;
  std::stringstream ipstream(ip);
  std::string hextet;
  size_t hextet_count = 0;

  while (std::getline(ipstream, hextet, ':')) {
    int32_t val = std::stoi(hextet, 0, 16);

    if (val < 0 || val >= std::numeric_limits<uint16_t>::max()) {
      throw ParseError(
          std::format("Invalid ipv6 {}: hextet {} is too big", ip, hextet));
    }

    WriteUint16(res, static_cast<uint16_t>(val));
    hextet_count += 1;
  }

  if (hextet_count != 8) {
    throw ParseError(
        std::format("Invalid ipv6 {}: Ipv6 must have 8 hextets", ip));
  }

  return res.str();
} catch (const std::exception &ex) {
  throw ParseError(std::format("Invalid ipv6 {}: {}", ip, ex.what()));
}

std::string ParseResponse(const std::string &response, RecordType record_type) {
  switch (record_type) {
  case RecordType::A:
    return ParseResponseA(response);
  case RecordType::AAAA:
    return ParseResponseAAAA(response);
  }

  throw ParseError("Unmapped record type");
}

std::unordered_map<Question, std::vector<Answer>>
ParseConfig(const nlohmann::json &config) {
  std::unordered_map<Question, std::vector<Answer>> res;

  if (!config.is_object()) {
    throw ParseError(std::format("Error parsing config. Expected object"));
  }

  for (const auto &[domain, records] : config.items()) {
    if (!records.is_array()) {
      throw ParseError(std::format(
          "Error parsing records list for {}. Expected array", domain));
    }

    for (const auto &record : records) {
      if (!record.contains(kRecordTypeField)) {
        throw ParseError(std::format(
            "Domain {} contains records with no type specified.", domain));
      }
      if (!record.contains(kRecordResponseField)) {
        throw ParseError(std::format(
            "Domain {} contains records with no response specified.", domain));
      }

      const auto record_type = ParseRecordType(record[kRecordTypeField]);

      Question q{
          .domain = domain,
          .type = record_type,
          .cls = QueryClass::IN,
      };

      Answer a{
          .response = ParseResponse(record[kRecordResponseField], record_type),
          .ttl = kDefaultTtl,
      };

      if (res.contains(q)) {
        res[q].push_back(a);
      } else {
        res[q] = {a};
      }
    }
  }

  return res;
}

} // namespace

std::unordered_map<Question, std::vector<Answer>>
ParseConfig(const std::string &config) try {
  const auto json = nlohmann::json::parse(config);
  return ParseConfig(json);
} catch (const ParseError &ex) {
  throw;
} catch (const nlohmann::json::parse_error &ex) {
  throw ParseError(std::format("Config is not valid json: {}", ex.what()));
} catch (const nlohmann::json::exception &ex) {
  throw ParseError(std::format("Failed to parse config: {}", ex.what()));
} catch (const std::exception &ex) {
  throw ParseError(std::format("Unknown error parsing config: {}", ex.what()));
}

std::unordered_map<Question, std::vector<Answer>>
ReadAndParseConfig(const std::string &path) try {
  std::ifstream config(path);
  if (!config) {
    throw ParseError(std::format("Failed to open config file {}: {}", path,
                                 std::strerror(errno)));
  }
  const auto json = nlohmann::json::parse(config);
  return ParseConfig(json);
} catch (const ParseError &ex) {
  throw;
} catch (const nlohmann::json::parse_error &ex) {
  throw ParseError(std::format("Config is not valid json: {}", ex.what()));
} catch (const nlohmann::json::exception &ex) {
  throw ParseError(std::format("Failed to parse config: {}", ex.what()));
} catch (const std::exception &ex) {
  throw ParseError(std::format("Unknown error parsing config: {}", ex.what()));
}

} // namespace dns
