#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.hpp"

namespace http {

struct ParsingError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct Request {
  HttpVersion http_version;
  HttpMethod http_method;

  std::string target;
  std::string path;
  std::unordered_map<std::string, std::string> query_params;
  std::unordered_map<std::string, std::string> headers;
  std::string content;
};

class RequestParser;

std::unique_ptr<RequestParser> CreateRequestParser();

class IRequestParserState {
public:
  virtual std::unique_ptr<IRequestParserState>
  Read(const std::string &data) = 0;

  virtual std::optional<Request> Get() const = 0;

  ~IRequestParserState() = default;
};

class RequestParser {
  RequestParser(std::unique_ptr<IRequestParserState> state)
      : state_(std::move(state)) {}

  friend std::unique_ptr<RequestParser> CreateRequestParser();

public:
  void Read(const std::string &data) {
    state_ = state_->Read(data);
    const auto request = state_->Get();
    if (request.has_value()) {
      requests_.push_back(request.value());
    }
  }

  std::vector<Request> Get() const { return requests_; }

  void Clear() { requests_.clear(); }

  std::vector<Request> Drain() {
    const auto requests = requests_;
    requests_.clear();
    return requests;
  }

private:
  std::unique_ptr<IRequestParserState> state_;
  std::vector<Request> requests_;
};

} // namespace http
