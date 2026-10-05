#include "request.hpp"

namespace http {

namespace {

class ReadingMethodState : public IRequestParserState {
  std::unique_ptr<IRequestParserState> Read(const std::string &data) override {
    data_ += data;

    if (data_.starts_with("POST")) {
      // TODO
    }

    return std::unique_ptr<IRequestParserState>(this);
  }

  std::optional<Request> Get() const override { return std::nullopt; }

private:
  std::string data_;
};

} // namespace

std::unique_ptr<RequestParser> CreateRequestParser() {
  return std::unique_ptr<RequestParser>(
      new RequestParser(std::make_unique<ReadingMethodState>()));
}

} // namespace http
