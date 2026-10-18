#include "config.hpp"

#include <gtest/gtest.h>

#include <format>
#include <string>

namespace dns {
namespace {

const auto kDomain = std::string{"example.com"};

std::string MakeConfig(const std::string &type, const std::string &response) {
  return std::format("{{\"{}\": [{{\"type\": \"{}\", \"response\": \"{}\"}}]}}",
                     kDomain, type, response);
}

TEST(ParseConfig, StoresFinalIpv4Representation) {
  const auto config = ParseConfig(MakeConfig("A", "127.0.0.1"));

  Question q{kDomain, RecordType::A, QueryClass::IN};
  ASSERT_TRUE(config.contains(q));
  ASSERT_EQ(config.at(q).size(), 1u);
  EXPECT_EQ(config.at(q)[0].response, std::string("\x7f\x00\x00\x01", 4));
  EXPECT_EQ(config.at(q)[0].ttl, 60);
}

TEST(ParseConfig, StoresFinalIpv6Representation) {
  const auto config = ParseConfig(MakeConfig("AAAA", "2001:db8:0:0:0:0:0:1"));

  Question q{kDomain, RecordType::AAAA, QueryClass::IN};
  ASSERT_TRUE(config.contains(q));
  ASSERT_EQ(config.at(q).size(), 1u);
  EXPECT_EQ(config.at(q)[0].response,
            std::string("\x20\x01\x0d\xb8\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x01",
                        16));
  EXPECT_EQ(config.at(q)[0].ttl, 60);
}

TEST(ParseConfig, MalformedIpv4ResponseThrows) {
  EXPECT_THROW(ParseConfig(MakeConfig("A", "127.0.0")), ParseError);
  EXPECT_THROW(ParseConfig(MakeConfig("A", "127.0.0.1.1")), ParseError);
  EXPECT_THROW(ParseConfig(MakeConfig("A", "300.0.0.1")), ParseError);
  EXPECT_THROW(ParseConfig(MakeConfig("A", "127.0.a.1")), ParseError);
}

TEST(ParseConfig, MalformedIpv6ResponseThrows) {
  EXPECT_THROW(ParseConfig(MakeConfig("AAAA", "2001:db8::1")), ParseError);
  EXPECT_THROW(ParseConfig(MakeConfig("AAAA", "1:2:3:4:5:6:7:8:9")),
               ParseError);
  EXPECT_THROW(ParseConfig(MakeConfig("AAAA", "12345:2:3:4:5:6:7:8")),
               ParseError);
  EXPECT_THROW(ParseConfig(MakeConfig("AAAA", "zzzz:2:3:4:5:6:7:8")),
               ParseError);
}

TEST(ParseConfig, ResponseAddressMismatchingTypeThrows) {
  EXPECT_THROW(ParseConfig(MakeConfig("AAAA", "127.0.0.1")), ParseError);
  EXPECT_THROW(ParseConfig(MakeConfig("A", "2001:db8:0:0:0:0:0:1")),
               ParseError);
}

TEST(ParseConfig, TopLevelNotObjectThrows) {
  EXPECT_THROW(ParseConfig("[1, 2, 3]"), ParseError);
  EXPECT_THROW(ParseConfig("\"just a string\""), ParseError);
}

TEST(ParseConfig, RecordsListNotArrayThrows) {
  EXPECT_THROW(ParseConfig("{\"example.com\": \"127.0.0.1\"}"), ParseError);
  EXPECT_THROW(ParseConfig("{\"example.com\": {\"type\": \"A\"}}"), ParseError);
}

TEST(ParseConfig, RecordMissingTypeThrows) {
  EXPECT_THROW(ParseConfig("{\"example.com\": [{\"response\": \"127.0.0.1\"}]}"),
               ParseError);
}

TEST(ParseConfig, RecordMissingResponseThrows) {
  EXPECT_THROW(ParseConfig("{\"example.com\": [{\"type\": \"A\"}]}"),
               ParseError);
}

TEST(ParseConfig, UnknownRecordTypeThrows) {
  EXPECT_THROW(
      ParseConfig("{\"example.com\": [{\"type\": \"UNKNOWN_TYPE\", \"response\": \"x\"}]}"),
      ParseError);
}

TEST(ParseConfig, InvalidJsonThrows) {
  EXPECT_THROW(ParseConfig("not json at all"), ParseError);
  EXPECT_THROW(ParseConfig("{\"example.com\": [}"), ParseError);
}

} // namespace
} // namespace dns
