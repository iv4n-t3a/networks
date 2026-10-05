// Tests for the HTTP request parser (http::RequestParser in request.hpp).
//
// These tests define the expected behavior of the parser and are currently
// RED: src/request.cpp is a stub that never produces a Request. They should
// go green once the parser is implemented.
//
// Spec pinned by these tests (per project decisions):
//  - Lenient parsing: LF-only line endings, extra spaces in the request line
//    and a missing Host header in HTTP/1.1 are accepted. ParsingError is
//    raised only for structurally unparseable input.
//  - Transfer-Encoding: chunked bodies are decoded into Request::content.
//  - Query parameter values are percent-decoded; key-only params map to "".
//  - Header keys are lowercased; header values are trimmed.

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "request.hpp"

namespace {

using http::Request;

// Feeds `data` in a single Read call and requires exactly one completed
// request, which is written to *out.
void ParseOne(const std::string &data, Request *out) {
  auto parser = http::CreateRequestParser();
  parser->Read(data);
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), 1u) << "input: " << data;
  *out = std::move(requests.front());
}

// Feeds each chunk with a separate Read call; requires exactly one completed
// request at the end.
void ParseOneInChunks(const std::vector<std::string> &chunks, Request *out) {
  auto parser = http::CreateRequestParser();
  for (const auto &chunk : chunks) {
    parser->Read(chunk);
  }
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), 1u);
  *out = std::move(requests.front());
}

// Feeds chunks in order; requires `count` completed requests at the end.
void ParseMany(const std::vector<std::string> &chunks, size_t count,
               std::vector<Request> *out) {
  auto parser = http::CreateRequestParser();
  for (const auto &chunk : chunks) {
    parser->Read(chunk);
  }
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), count);
  *out = std::move(requests);
}

// Feeds chunks in order; requires that no request has completed yet.
void ExpectIncomplete(const std::vector<std::string> &chunks) {
  auto parser = http::CreateRequestParser();
  for (const auto &chunk : chunks) {
    parser->Read(chunk);
  }
  EXPECT_TRUE(parser->Get().empty());
}

// Feeds `data` in a single Read call and requires a ParsingError.
void ExpectError(const std::string &data) {
  auto parser = http::CreateRequestParser();
  EXPECT_THROW(parser->Read(data), http::ParsingError);
}

} // namespace

// ---------------------------------------------------------------------------
// HTTP/0.9: a single request line, no HTTP version, no headers, no body.
// ---------------------------------------------------------------------------

TEST(RequestParserHttp09, Minimal) {
  Request request;
  ParseOne("GET /path\r\n", &request);
  EXPECT_EQ(request.http_version, http::HttpVersion::V09);
  EXPECT_EQ(request.http_method, http::HttpMethod::GET);
  EXPECT_EQ(request.target, "/path");
  EXPECT_EQ(request.path, "/path");
  EXPECT_TRUE(request.query_params.empty());
  EXPECT_TRUE(request.headers.empty());
  EXPECT_TRUE(request.content.empty());
}

TEST(RequestParserHttp09, RootPath) {
  Request request;
  ParseOne("GET /\r\n", &request);
  EXPECT_EQ(request.target, "/");
  EXPECT_EQ(request.path, "/");
}

TEST(RequestParserHttp09, QueryParams) {
  Request request;
  ParseOne("GET /search?q=hello%20world&flag\r\n", &request);
  EXPECT_EQ(request.path, "/search");
  EXPECT_EQ(request.query_params.size(), 2u);
  EXPECT_EQ(request.query_params.at("q"), "hello world");
  EXPECT_EQ(request.query_params.at("flag"), "");
}

TEST(RequestParserHttp09, LfOnly) {
  Request request;
  ParseOne("GET /path\n", &request);
  EXPECT_EQ(request.http_version, http::HttpVersion::V09);
  EXPECT_EQ(request.path, "/path");
}

TEST(RequestParserHttp09, SplitAcrossChunks) {
  auto parser = http::CreateRequestParser();
  parser->Read("GET /pa");
  EXPECT_TRUE(parser->Get().empty());
  parser->Read("th\r\n");
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), 1u);
  EXPECT_EQ(requests.front().path, "/path");
}

TEST(RequestParserHttp09, ByteByByte) {
  const std::string data = "GET /path\r\n";
  auto parser = http::CreateRequestParser();
  for (const char c : data) {
    parser->Read(std::string(1, c));
  }
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), 1u);
  EXPECT_EQ(requests.front().path, "/path");
}

TEST(RequestParserHttp09, UnterminatedRequestLineIncomplete) {
  // A Read boundary must not complete a request; only the terminator does.
  ExpectIncomplete({"GET /path"});
}

TEST(RequestParserHttp09, TerminatorInLaterRead) {
  auto parser = http::CreateRequestParser();
  parser->Read("GET /path");
  EXPECT_TRUE(parser->Get().empty());
  parser->Read("\r\n");
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), 1u);
  EXPECT_EQ(requests.front().path, "/path");
}

// ---------------------------------------------------------------------------
// HTTP/1.0: request line with version, header section, optional body.
// ---------------------------------------------------------------------------

TEST(RequestParserHttp10, Minimal) {
  Request request;
  ParseOne("GET /path HTTP/1.0\r\n\r\n", &request);
  EXPECT_EQ(request.http_version, http::HttpVersion::V10);
  EXPECT_EQ(request.http_method, http::HttpMethod::GET);
  EXPECT_EQ(request.target, "/path");
  EXPECT_EQ(request.path, "/path");
  EXPECT_TRUE(request.headers.empty());
  EXPECT_TRUE(request.content.empty());
}

TEST(RequestParserHttp10, WithHeaders) {
  Request request;
  ParseOne("GET / HTTP/1.0\r\nHost: example.com\r\nAccept: text/html\r\n\r\n",
           &request);
  EXPECT_EQ(request.headers.size(), 2u);
  EXPECT_EQ(request.headers.at("host"), "example.com");
  EXPECT_EQ(request.headers.at("accept"), "text/html");
}

TEST(RequestParserHttp10, MixedCaseHeaderKeys) {
  Request request;
  ParseOne("GET / HTTP/1.0\r\nHOST: a\r\nX-Custom-Header: v\r\n\r\n", &request);
  // Keys are lowercased, so lookup is case-insensitive.
  EXPECT_EQ(request.headers.size(), 2u);
  EXPECT_EQ(request.headers.at("host"), "a");
  EXPECT_EQ(request.headers.at("x-custom-header"), "v");
}

TEST(RequestParserHttp10, TightColon) {
  Request request;
  ParseOne("GET / HTTP/1.0\r\nX:v\r\n\r\n", &request);
  EXPECT_EQ(request.headers.at("x"), "v");
}

TEST(RequestParserHttp10, EmptyHeaderValue) {
  Request request;
  ParseOne("GET / HTTP/1.0\r\nX-Empty:\r\n\r\n", &request);
  EXPECT_EQ(request.headers.at("x-empty"), "");
}

TEST(RequestParserHttp10, HeaderValueWhitespaceTrimmed) {
  Request request;
  ParseOne("GET / HTTP/1.0\r\nX-Space:   padded value  \r\n\r\n", &request);
  EXPECT_EQ(request.headers.at("x-space"), "padded value");
}

TEST(RequestParserHttp10, LfOnlyEndings) {
  Request request;
  ParseOne("GET / HTTP/1.0\nHost: a\nX: y\n\n", &request);
  EXPECT_EQ(request.headers.size(), 2u);
  EXPECT_EQ(request.headers.at("host"), "a");
  EXPECT_EQ(request.headers.at("x"), "y");
}

TEST(RequestParserHttp10, ContentLengthBody) {
  Request request;
  ParseOne("POST /submit HTTP/1.0\r\nContent-Length: 5\r\n\r\nhello",
           &request);
  EXPECT_EQ(request.http_method, http::HttpMethod::POST);
  EXPECT_EQ(request.content, "hello");
}

TEST(RequestParserHttp10, ContentLengthZero) {
  Request request;
  ParseOne("POST / HTTP/1.0\r\nContent-Length: 0\r\n\r\n", &request);
  EXPECT_TRUE(request.content.empty());
}

TEST(RequestParserHttp10, ContentLengthBodySplitAcrossReads) {
  Request request;
  ParseOneInChunks({"POST /submit HTTP/1.0\r\nContent-Length: 5\r\n\r\nhe",
                    "llo"},
                   &request);
  EXPECT_EQ(request.content, "hello");
}

TEST(RequestParserHttp10, SplitMidMethod) {
  Request request;
  ParseOneInChunks({"GE", "T / HTTP/1.0\r\n\r\n"}, &request);
  EXPECT_EQ(request.http_method, http::HttpMethod::GET);
  EXPECT_EQ(request.path, "/");
}

TEST(RequestParserHttp10, SplitMidHeader) {
  Request request;
  ParseOneInChunks({"GET / HTTP/1.0\r\nHos", "t: x\r\n\r\n"}, &request);
  EXPECT_EQ(request.headers.at("host"), "x");
}

TEST(RequestParserHttp10, LowercaseContentLengthHeader) {
  // Special headers are matched case-insensitively (keys are normalized).
  Request request;
  ParseOne("POST / HTTP/1.0\r\ncontent-length: 5\r\n\r\nhello", &request);
  EXPECT_EQ(request.content, "hello");
}

// ---------------------------------------------------------------------------
// HTTP/1.1
// ---------------------------------------------------------------------------

TEST(RequestParserHttp11, MinimalWithHost) {
  Request request;
  ParseOne("GET / HTTP/1.1\r\nHost: a\r\n\r\n", &request);
  EXPECT_EQ(request.http_version, http::HttpVersion::V11);
  EXPECT_EQ(request.headers.at("host"), "a");
}

TEST(RequestParserHttp11, MissingHostLenient) {
  // HTTP/1.1 requires Host, but the parser is lenient: no error.
  Request request;
  ParseOne("GET / HTTP/1.1\r\n\r\n", &request);
  EXPECT_EQ(request.http_version, http::HttpVersion::V11);
  EXPECT_TRUE(request.headers.empty());
}

TEST(RequestParserHttp11, AbsoluteFormTarget) {
  Request request;
  ParseOne("GET http://example.com/a/b?x=1 HTTP/1.1\r\nHost: example.com\r\n\r\n",
           &request);
  EXPECT_EQ(request.target, "http://example.com/a/b?x=1");
  EXPECT_EQ(request.path, "/a/b");
  EXPECT_EQ(request.query_params.size(), 1u);
  EXPECT_EQ(request.query_params.at("x"), "1");
}

TEST(RequestParserHttp11, UrlEncodedQuery) {
  Request request;
  ParseOne("GET /search?q=hello%20world HTTP/1.1\r\nHost: a\r\n\r\n",
           &request);
  // The path itself is not decoded, only query parameter values are.
  EXPECT_EQ(request.path, "/search");
  EXPECT_EQ(request.query_params.size(), 1u);
  EXPECT_EQ(request.query_params.at("q"), "hello world");
}

TEST(RequestParserHttp11, KeyOnlyQueryParam) {
  Request request;
  ParseOne("GET /?flag HTTP/1.1\r\nHost: a\r\n\r\n", &request);
  EXPECT_EQ(request.query_params.size(), 1u);
  EXPECT_EQ(request.query_params.at("flag"), "");
}

TEST(RequestParserHttp11, DuplicateQueryKeys) {
  // Last occurrence wins.
  Request request;
  ParseOne("GET /?a=1&a=2 HTTP/1.1\r\nHost: a\r\n\r\n", &request);
  EXPECT_EQ(request.query_params.size(), 1u);
  EXPECT_EQ(request.query_params.at("a"), "2");
}

TEST(RequestParserHttp11, EmptyQueryValue) {
  Request request;
  ParseOne("GET /?a= HTTP/1.1\r\nHost: a\r\n\r\n", &request);
  EXPECT_EQ(request.query_params.size(), 1u);
  EXPECT_EQ(request.query_params.at("a"), "");
}

TEST(RequestParserHttp11, PlusSignNotDecoded) {
  // RFC 3986: '+' is a literal in a query string; only %XX is decoded.
  Request request;
  ParseOne("GET /?q=a+b HTTP/1.1\r\nHost: a\r\n\r\n", &request);
  EXPECT_EQ(request.query_params.at("q"), "a+b");
}

TEST(RequestParserHttp11, ExtraSpacesInRequestLine) {
  // Lenient: multiple spaces between request-line tokens are accepted.
  Request request;
  ParseOne("GET  /  HTTP/1.1\r\nHost: a\r\n\r\n", &request);
  EXPECT_EQ(request.http_version, http::HttpVersion::V11);
  EXPECT_EQ(request.path, "/");
}

// ---------------------------------------------------------------------------
// Chunked transfer encoding (HTTP/1.1)
// ---------------------------------------------------------------------------

namespace {

const std::string kChunkedHeader =
    "POST / HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: chunked\r\n\r\n";

} // namespace

TEST(RequestParserChunked, SingleChunk) {
  Request request;
  ParseOne(kChunkedHeader + "5\r\nhello\r\n0\r\n\r\n", &request);
  EXPECT_EQ(request.content, "hello");
}

TEST(RequestParserChunked, MultipleChunks) {
  Request request;
  ParseOne(kChunkedHeader + "4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n", &request);
  EXPECT_EQ(request.content, "Wikipedia");
}

TEST(RequestParserChunked, ChunkExtensions) {
  // Chunk extensions are ignored.
  Request request;
  ParseOne(kChunkedHeader + "5;ext=1;foo=bar\r\nhello\r\n0\r\n\r\n",
           &request);
  EXPECT_EQ(request.content, "hello");
}

TEST(RequestParserChunked, SplitAcrossReads) {
  auto parser = http::CreateRequestParser();
  parser->Read("POST / HTTP/1.1\r\nHost: a\r\nTransfer-Encod");
  EXPECT_TRUE(parser->Get().empty());
  parser->Read("ing: chunked\r\n\r\n5\r\nhel");
  EXPECT_TRUE(parser->Get().empty());
  parser->Read("lo\r\n0\r\n\r\n");
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), 1u);
  EXPECT_EQ(requests.front().content, "hello");
}

TEST(RequestParserChunked, UppercaseHexChunkSize) {
  Request request;
  ParseOne(kChunkedHeader + "A\r\n0123456789\r\n0\r\n\r\n", &request);
  EXPECT_EQ(request.content, "0123456789");
}

TEST(RequestParserChunked, MissingTerminatingChunk) {
  // Without the terminating 0-chunk the request stays pending; a request line
  // where a chunk-size line was expected is unparseable.
  auto parser = http::CreateRequestParser();
  parser->Read(kChunkedHeader + "4\r\ntest\r\n");
  EXPECT_TRUE(parser->Get().empty());
  EXPECT_THROW(parser->Read("GET / HTTP/1.1\r\nHost: a\r\n\r\n"),
               http::ParsingError);
}

TEST(RequestParserChunked, NonNumericChunkSize) {
  ExpectError(kChunkedHeader + "zz\r\nabc\r\n0\r\n\r\n");
}

// ---------------------------------------------------------------------------
// Malformed input
// ---------------------------------------------------------------------------

TEST(RequestParserErrors, EmptyReadIsNoOp) {
  ExpectIncomplete({""});
}

TEST(RequestParserErrors, GarbageRequestLine) {
  ExpectError("THIS IS GARBAGE\r\n\r\n");
}

TEST(RequestParserErrors, TooFewTokensRequestLine) {
  ExpectError("GET\r\n\r\n");
}

TEST(RequestParserErrors, UnsupportedMethodAsteriskForm) {
  // OPTIONS is not in the HttpMethod enum, so this is unparseable. Flip this
  // test to expect target == "*" if OPTIONS is added to the enum.
  ExpectError("OPTIONS * HTTP/1.1\r\nHost: a\r\n\r\n");
}

TEST(RequestParserErrors, HeaderWithoutColon) {
  ExpectError("GET / HTTP/1.0\r\nNoColonHere\r\n\r\n");
}

TEST(RequestParserErrors, NonNumericContentLength) {
  ExpectError("POST / HTTP/1.0\r\nContent-Length: abc\r\n\r\n");
}

TEST(RequestParserErrors, TooManyTokensRequestLine) {
  ExpectError("GET / HTTP/1.1 extra\r\n\r\n");
}

TEST(RequestParserErrors, BadHttpVersion) {
  ExpectError("GET / HTTP/9.9\r\n\r\n");
}

// ---------------------------------------------------------------------------
// Keep-alive / pipelining: multiple requests per connection
// ---------------------------------------------------------------------------

TEST(RequestParserPipelining, TwoRequestsOneBuffer) {
  auto parser = http::CreateRequestParser();
  parser->Read("GET /a HTTP/1.1\r\nHost: a\r\n\r\n"
               "GET /b HTTP/1.1\r\nHost: b\r\n\r\n");
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), 2u);
  EXPECT_EQ(requests[0].path, "/a");
  EXPECT_EQ(requests[0].headers.at("host"), "a");
  EXPECT_EQ(requests[1].path, "/b");
  EXPECT_EQ(requests[1].headers.at("host"), "b");
}

TEST(RequestParserPipelining, SplitAcrossReads) {
  auto parser = http::CreateRequestParser();
  parser->Read("GET /a HTTP/1.1\r\nHost: a\r\n\r\nGET /b HT");
  EXPECT_EQ(parser->Get().size(), 1u);
  parser->Read("TP/1.1\r\nHost: b\r\n\r\n");
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), 2u);
  EXPECT_EQ(requests[0].path, "/a");
  EXPECT_EQ(requests[1].path, "/b");
}

TEST(RequestParserPipelining, DrainReturnsAndClears) {
  auto parser = http::CreateRequestParser();
  parser->Read("GET /a HTTP/1.1\r\nHost: a\r\n\r\n"
               "GET /b HTTP/1.1\r\nHost: b\r\n\r\n");
  auto first = parser->Drain();
  ASSERT_EQ(first.size(), 2u);
  EXPECT_TRUE(parser->Get().empty());
  EXPECT_TRUE(parser->Drain().empty());
}

TEST(RequestParserPipelining, ClearDropsCompletedButKeepsParserState) {
  auto parser = http::CreateRequestParser();
  parser->Read("GET /a HTTP/1.1\r\nHost: a\r\n\r\n"
               "GET /b HTTP/1.1\r\nHost: b\r\n\r\n");
  parser->Clear();
  EXPECT_TRUE(parser->Get().empty());
  parser->Read("GET /c HTTP/1.1\r\nHost: c\r\n\r\n");
  auto requests = parser->Drain();
  ASSERT_EQ(requests.size(), 1u);
  EXPECT_EQ(requests.front().path, "/c");
}

TEST(RequestParserPipelining, GetDoesNotClear) {
  auto parser = http::CreateRequestParser();
  parser->Read("GET /a HTTP/1.1\r\nHost: a\r\n\r\n"
               "GET /b HTTP/1.1\r\nHost: b\r\n\r\n");
  EXPECT_EQ(parser->Get().size(), 2u);
  EXPECT_EQ(parser->Get().size(), 2u);
}

TEST(RequestParserPipelining, ReadEmptyNoOpAfterComplete) {
  auto parser = http::CreateRequestParser();
  parser->Read("GET /a HTTP/1.1\r\nHost: a\r\n\r\n");
  parser->Read("");
  EXPECT_EQ(parser->Get().size(), 1u);
}
