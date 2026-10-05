#pragma once

namespace http {

enum class HttpVersion {
  V09,
  V10,
  V11,
  V2,
  V3,
};

enum class HttpMethod {
  GET,
  POST,
  PUT,
  DELETE,
  PATCH,
  HEAD,
  QUERY,
};

} // namespace http
