#include "request_parser.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

namespace dns {
namespace {

std::string MakeQuery(uint16_t id, uint16_t questions, const std::string &body) {
  std::string data;
  data.push_back(static_cast<char>(id >> 8));
  data.push_back(static_cast<char>(id & 0xFF));
  data.push_back(0x01); // flags: RD set, as sent by dig
  data.push_back(0);
  data.push_back(static_cast<char>(questions >> 8));
  data.push_back(static_cast<char>(questions & 0xFF));
  data.append(6, '\0'); // answers, authority, additional
  data += body;
  return data;
}

std::string Question(const std::string &labels, uint8_t type, uint8_t cls) {
  std::string q = labels;
  q.push_back(0);
  q.push_back(0);
  q.push_back(static_cast<char>(type));
  q.push_back(0);
  q.push_back(static_cast<char>(cls));
  return q;
}

const std::string kExampleLabels = std::string("\x07") + "example" + "\x03" + "com";

TEST(RequestParser,  Identification) {
  std::string data = MakeQuery(0x1234, 0, "");
  RequestParser parser(data);
  EXPECT_EQ(parser.Identification(), 0x1234);
}

TEST(RequestParser,  NoQuestions) {
  std::string data = MakeQuery(1, 0, "");
  RequestParser parser(data);
  EXPECT_FALSE(parser.GetNextQuestion().has_value());
}

TEST(RequestParser,  SingleQuestionDomain) {
  std::string data = MakeQuery(1, 1, Question(kExampleLabels, 1, 1));
  RequestParser parser(data);

  auto q = parser.GetNextQuestion();
  ASSERT_TRUE(q.has_value());
  EXPECT_EQ(q->domain, "example.com");
  EXPECT_FALSE(parser.GetNextQuestion().has_value());
}

TEST(RequestParser,  SingleQuestionTypeAndClass) {
  std::string data = MakeQuery(1, 1, Question(kExampleLabels, 28, 1));
  RequestParser parser(data);

  auto q = parser.GetNextQuestion();
  ASSERT_TRUE(q.has_value());
  EXPECT_EQ(q->type, RecordType::AAAA);
  EXPECT_EQ(q->cls, QueryClass::IN);
}

TEST(RequestParser,  MultipleQuestions) {
  std::string body = Question(kExampleLabels, 1, 1) +
                     Question(std::string("\x03") + "foo" + "\x03" + "org", 28, 1);
  std::string data = MakeQuery(1, 2, body);
  RequestParser parser(data);

  auto first = parser.GetNextQuestion();
  auto second = parser.GetNextQuestion();
  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(first->domain, "example.com");
  EXPECT_EQ(first->type, RecordType::A);
  EXPECT_EQ(second->domain, "foo.org");
  EXPECT_EQ(second->type, RecordType::AAAA);
  EXPECT_FALSE(parser.GetNextQuestion().has_value());
}

TEST(RequestParser,  ResetRestartsIteration) {
  std::string data = MakeQuery(1, 1, Question(kExampleLabels, 1, 1));
  RequestParser parser(data);

  ASSERT_TRUE(parser.GetNextQuestion().has_value());
  ASSERT_FALSE(parser.GetNextQuestion().has_value());

  parser.Reset();
  auto q = parser.GetNextQuestion();
  ASSERT_TRUE(q.has_value());
  EXPECT_EQ(q->domain, "example.com");
}

} // namespace
} // namespace dns
