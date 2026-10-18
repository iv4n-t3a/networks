#include "message_builder.hpp"
#include "message_parser.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace dns {
namespace {

constexpr size_t kHeaderSize = 12;

uint8_t GetUInt8(const std::string &s, size_t i) {
  return static_cast<uint8_t>(s.at(i));
}

uint16_t GetUInt16(const std::string &s, size_t i) {
  return (GetUInt8(s, i) << 8) | GetUInt8(s, i + 1);
}

TEST(MessageBuilder, EmptyResponseIsHeaderOnly) {
  MessageBuilder builder(7);
  std::string res = builder.BuildResponse();
  ASSERT_EQ(res.size(), kHeaderSize);
  EXPECT_EQ(GetUInt16(res, 4), 0);  // QDCOUNT
  EXPECT_EQ(GetUInt16(res, 6), 0);  // ANCOUNT
  EXPECT_EQ(GetUInt16(res, 8), 0);  // NSCOUNT
  EXPECT_EQ(GetUInt16(res, 10), 0); // ARCOUNT
}

TEST(MessageBuilder, FlagsMarkMessageAsResponse) {
  MessageBuilder builder(1);
  std::string res = builder.BuildResponse();
  EXPECT_EQ(GetUInt8(res, 2), 0x80);
  EXPECT_EQ(GetUInt8(res, 3), 0x00);
}

TEST(MessageBuilder, EchoesQueryIdentification) {
  std::string query("\x12\x34\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00", 12);
  MessageParser parser(query);
  MessageBuilder builder(parser.Identification());
  std::string res = builder.BuildResponse();
  EXPECT_EQ(GetUInt8(res, 0), 0x12);
  EXPECT_EQ(GetUInt8(res, 1), 0x34);
}

TEST(MessageBuilder, CountsAreInNetworkByteOrder) {
  MessageBuilder builder(1);
  builder.AddQuestion("a.com", RecordType::A, QueryClass::IN);
  builder.AddQuestion("b.com", RecordType::AAAA, QueryClass::IN);
  builder.AddResponse("a.com", RecordType::A, QueryClass::IN, 0, "127.0.0.1");
  std::string res = builder.BuildResponse();

  EXPECT_EQ(GetUInt8(res, 4), 0);
  EXPECT_EQ(GetUInt8(res, 5), 2);
  EXPECT_EQ(GetUInt8(res, 6), 0);
  EXPECT_EQ(GetUInt8(res, 7), 1);
}

TEST(MessageBuilder, AddQuestionEncodesDomainTypeAndClass) {
  MessageBuilder builder(1);
  builder.AddQuestion("example.com", RecordType::A, QueryClass::IN);
  std::string res = builder.BuildResponse();

  std::string expected = std::string("\x07") + "example" + "\x03" + "com" +
                         std::string("\x00", 1) + std::string("\x00\x01", 2) +
                         std::string("\x00\x01", 2);
  EXPECT_EQ(res.substr(kHeaderSize), expected);
}

TEST(MessageBuilder, AddResponseEncodesRecord) {
  MessageBuilder builder(1);
  builder.AddResponse("a.io", RecordType::A, QueryClass::IN, 0x01020304, "127.0.0.1");
  std::string res = builder.BuildResponse();

  std::string expected = std::string("\x01") + "a" + "\x02" + "io" +
                         std::string("\x00", 1) + std::string("\x00\x01", 2) +
                         std::string("\x00\x01", 2) +
                         std::string("\x01\x02\x03\x04", 4) +
                         std::string("\x00\x04", 2) +
                         std::string("\x7f\x00\x00\x01", 4);
  EXPECT_EQ(res.substr(kHeaderSize), expected);
}

TEST(MessageBuilder, ReplyQuestionMatchesQuery) {
  std::string query("\x12\x34\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00", 12);
  query += std::string("\x07") + "example" + "\x03" + "com" +
           std::string("\x00\x00\x01\x00\x01", 5);

  MessageParser parser(query);
  MessageBuilder builder(parser.Identification());
  for (auto q = parser.GetNextQuestion(); q; q = parser.GetNextQuestion()) {
    builder.AddQuestion(q->domain, q->type, q->cls);
  }
  std::string res = builder.BuildResponse();

  EXPECT_EQ(res.substr(kHeaderSize), query.substr(kHeaderSize));
}

TEST(MessageBuilder, AddResponseEncodesAaaaRecord) {
  MessageBuilder builder(1);
  builder.AddResponse("a.io", RecordType::AAAA, QueryClass::IN, 0x01020304,
                      "2001:db8:0:0:0:0:0:1");
  std::string res = builder.BuildResponse();

  std::string expected = std::string("\x01") + "a" + "\x02" + "io" +
                         std::string("\x00", 1) + std::string("\x00\x1c", 2) +
                         std::string("\x00\x01", 2) +
                         std::string("\x01\x02\x03\x04", 4) +
                         std::string("\x00\x10", 2) +
                         std::string("\x20\x01\x0d\xb8\x00\x00\x00\x00"
                                     "\x00\x00\x00\x00\x00\x00\x00\x01",
                                     16);
  EXPECT_EQ(res.substr(kHeaderSize), expected);
}

TEST(MessageBuilder, MalformedIpv4ResponseThrows) {
  MessageBuilder builder(1);

  EXPECT_THROW(builder.AddResponse("a.io", RecordType::A, QueryClass::IN, 60,
                                   "127.0.0"),
               FailedToEncodeResponse);
  EXPECT_THROW(builder.AddResponse("a.io", RecordType::A, QueryClass::IN, 60,
                                   "127.0.0.1.1"),
               FailedToEncodeResponse);
  EXPECT_THROW(builder.AddResponse("a.io", RecordType::A, QueryClass::IN, 60,
                                   "300.0.0.1"),
               FailedToEncodeResponse);
  EXPECT_THROW(builder.AddResponse("a.io", RecordType::A, QueryClass::IN, 60,
                                   "127.0.a.1"),
               FailedToEncodeResponse);
}

TEST(MessageBuilder, MalformedIpv6ResponseThrows) {
  MessageBuilder builder(1);

  EXPECT_THROW(builder.AddResponse("a.io", RecordType::AAAA, QueryClass::IN,
                                   60, "2001:db8::1"),
               FailedToEncodeResponse);
  EXPECT_THROW(builder.AddResponse("a.io", RecordType::AAAA, QueryClass::IN,
                                   60, "1:2:3:4:5:6:7:8:9"),
               FailedToEncodeResponse);
  EXPECT_THROW(builder.AddResponse("a.io", RecordType::AAAA, QueryClass::IN,
                                   60, "12345:2:3:4:5:6:7:8"),
               FailedToEncodeResponse);
  EXPECT_THROW(builder.AddResponse("a.io", RecordType::AAAA, QueryClass::IN,
                                   60, "zzzz:2:3:4:5:6:7:8"),
               FailedToEncodeResponse);
}

TEST(MessageBuilder, ResponseAddressMismatchingTypeThrows) {
  MessageBuilder builder(1);

  EXPECT_THROW(builder.AddResponse("a.io", RecordType::AAAA, QueryClass::IN,
                                   60, "127.0.0.1"),
               FailedToEncodeResponse);
  EXPECT_THROW(builder.AddResponse("a.io", RecordType::A, QueryClass::IN, 60,
                                   "2001:db8:0:0:0:0:0:1"),
               FailedToEncodeResponse);
}

} // namespace
} // namespace dns
