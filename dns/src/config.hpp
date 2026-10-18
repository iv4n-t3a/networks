#pragma once

#include "common.hpp"

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace dns {

struct ParseError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

std::unordered_map<Question, std::vector<Answer>> ParseConfig(const std::string &config);

std::unordered_map<Question, std::vector<Answer>> ReadAndParseConfig(const std::string &path);

} // namespace dns
