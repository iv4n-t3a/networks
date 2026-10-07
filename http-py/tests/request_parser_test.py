import pytest

from request import ParsingError, parse_http_request


class TestRequestParserHttp09:
    def test_minimal(self):
        [request] = parse_http_request("GET /path\r\n")
        assert request.http_version == "0.9"
        assert request.http_method == "GET"
        assert request.target == "/path"
        assert request.path == "/path"
        assert request.query_params == {}
        assert request.headers == {}
        assert request.content == ""

    def test_root_path(self):
        [request] = parse_http_request("GET /\r\n")
        assert request.target == "/"
        assert request.path == "/"

    def test_query_params(self):
        [request] = parse_http_request("GET /search?q=hello%20world&flag\r\n")
        assert request.path == "/search"
        assert len(request.query_params) == 2
        assert request.query_params["q"] == "hello world"
        assert request.query_params["flag"] == ""

    def test_lf_only(self):
        [request] = parse_http_request("GET /path\n")
        assert request.http_version == "0.9"
        assert request.path == "/path"

    def test_unterminated_request_line_incomplete(self):
        assert parse_http_request("GET /path") == []

    def test_split_across_reads(self):
        assert parse_http_request("GET /pa") == []
        [request] = parse_http_request("GET /path\r\n")
        assert request.path == "/path"

    def test_every_prefix_incomplete(self):
        data = "GET /path\r\n"
        for i in range(len(data)):
            assert parse_http_request(data[:i]) == []
        [request] = parse_http_request(data)
        assert request.path == "/path"


class TestRequestParserHttp10:
    def test_minimal(self):
        [request] = parse_http_request("GET /path HTTP/1.0\r\n\r\n")
        assert request.http_version == "1.0"
        assert request.http_method == "GET"
        assert request.target == "/path"
        assert request.path == "/path"
        assert request.headers == {}
        assert request.content == ""

    def test_with_headers(self):
        [request] = parse_http_request(
            "GET / HTTP/1.0\r\nHost: example.com\r\nAccept: text/html\r\n\r\n")
        assert len(request.headers) == 2
        assert request.headers["host"] == "example.com"
        assert request.headers["accept"] == "text/html"

    def test_mixed_case_header_keys(self):
        [request] = parse_http_request(
            "GET / HTTP/1.0\r\nHOST: a\r\nX-Custom-Header: v\r\n\r\n")

        assert len(request.headers) == 2
        assert request.headers["host"] == "a"
        assert request.headers["x-custom-header"] == "v"

    def test_tight_colon(self):
        [request] = parse_http_request("GET / HTTP/1.0\r\nX:v\r\n\r\n")
        assert request.headers["x"] == "v"

    def test_empty_header_value(self):
        [request] = parse_http_request("GET / HTTP/1.0\r\nX-Empty:\r\n\r\n")
        assert request.headers["x-empty"] == ""

    def test_header_value_whitespace_trimmed(self):
        [request] = parse_http_request("GET / HTTP/1.0\r\nX-Space:   padded value  \r\n\r\n")
        assert request.headers["x-space"] == "padded value"

    def test_lf_only_endings(self):
        [request] = parse_http_request("GET / HTTP/1.0\nHost: a\nX: y\n\n")
        assert len(request.headers) == 2
        assert request.headers["host"] == "a"
        assert request.headers["x"] == "y"

    def test_content_length_body(self):
        [request] = parse_http_request("POST /submit HTTP/1.0\r\nContent-Length: 5\r\n\r\nhello")
        assert request.http_method == "POST"
        assert request.content == "hello"

    def test_content_length_zero(self):
        [request] = parse_http_request("POST / HTTP/1.0\r\nContent-Length: 0\r\n\r\n")
        assert request.content == ""

    def test_content_length_body_incomplete(self):
        assert parse_http_request(
            "POST /submit HTTP/1.0\r\nContent-Length: 5\r\n\r\nhe") == []

    def test_split_mid_method(self):
        assert parse_http_request("GE") == []
        [request] = parse_http_request("GET / HTTP/1.0\r\n\r\n")
        assert request.http_method == "GET"
        assert request.path == "/"

    def test_split_mid_header(self):
        assert parse_http_request("GET / HTTP/1.0\r\nHos") == []
        [request] = parse_http_request("GET / HTTP/1.0\r\nHost: x\r\n\r\n")
        assert request.headers["host"] == "x"

    def test_lowercase_content_length_header(self):
        [request] = parse_http_request("POST / HTTP/1.0\r\ncontent-length: 5\r\n\r\nhello")
        assert request.content == "hello"

class TestRequestParserHttp11:
    def test_minimal_with_host(self):
        [request] = parse_http_request("GET / HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.http_version == "1.1"
        assert request.headers["host"] == "a"

    def test_missing_host_lenient(self):
        [request] = parse_http_request("GET / HTTP/1.1\r\n\r\n")
        assert request.http_version == "1.1"
        assert request.headers == {}

    def test_absolute_form_target(self):
        [request] = parse_http_request(
            "GET http://example.com/a/b?x=1 HTTP/1.1\r\nHost: example.com\r\n\r\n")
        assert request.target == "http://example.com/a/b?x=1"
        assert request.path == "/a/b"
        assert len(request.query_params) == 1
        assert request.query_params["x"] == "1"

    def test_url_encoded_query(self):
        [request] = parse_http_request("GET /search?q=hello%20world HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.path == "/search"
        assert len(request.query_params) == 1
        assert request.query_params["q"] == "hello world"

    def test_key_only_query_param(self):
        [request] = parse_http_request("GET /?flag HTTP/1.1\r\nHost: a\r\n\r\n")
        assert len(request.query_params) == 1
        assert request.query_params["flag"] == ""

    def test_duplicate_query_keys(self):
        [request] = parse_http_request("GET /?a=1&a=2 HTTP/1.1\r\nHost: a\r\n\r\n")
        assert len(request.query_params) == 1
        assert request.query_params["a"] == "2"

    def test_empty_query_value(self):
        [request] = parse_http_request("GET /?a= HTTP/1.1\r\nHost: a\r\n\r\n")
        assert len(request.query_params) == 1
        assert request.query_params["a"] == ""

    def test_plus_sign_not_decoded(self):
        [request] = parse_http_request("GET /?q=a+b HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.query_params["q"] == "a+b"

    def test_extra_spaces_in_request_line(self):
        [request] = parse_http_request("GET  /  HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.http_version == "1.1"
        assert request.path == "/"

    def test_asterisk_form_target(self):
        [request] = parse_http_request("OPTIONS * HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.http_method == "OPTIONS"
        assert request.target == "*"


CHUNKED_HEADER = "POST / HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: chunked\r\n\r\n"

class TestRequestParserChunked:
    def test_single_chunk(self):
        [request] = parse_http_request(CHUNKED_HEADER + "5\r\nhello\r\n0\r\n\r\n")
        assert request.content == "hello"

    def test_multiple_chunks(self):
        [request] = parse_http_request(CHUNKED_HEADER + "4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n")
        assert request.content == "Wikipedia"

    def test_chunk_extensions(self):
        [request] = parse_http_request(CHUNKED_HEADER + "5;ext=1;foo=bar\r\nhello\r\n0\r\n\r\n")
        assert request.content == "hello"

    def test_split_across_reads(self):
        data = CHUNKED_HEADER + "5\r\nhello\r\n0\r\n\r\n"
        assert parse_http_request(data[:-6]) == []
        assert parse_http_request(data[:-1]) == []
        [request] = parse_http_request(data)
        assert request.content == "hello"

    def test_uppercase_hex_chunk_size(self):
        [request] = parse_http_request(CHUNKED_HEADER + "A\r\n0123456789\r\n0\r\n\r\n")
        assert request.content == "0123456789"

    def test_missing_terminating_chunk(self):
        assert parse_http_request(CHUNKED_HEADER + "4\r\ntest\r\n") == []

    def test_non_numeric_chunk_size(self):
        with pytest.raises(ParsingError):
            parse_http_request(CHUNKED_HEADER + "zz\r\nabc\r\n0\r\n\r\n")


class TestRequestParserErrors:
    def test_empty_read_is_no_op(self):
        assert parse_http_request("") == []

    def test_garbage_request_line(self):
        with pytest.raises(ParsingError):
            parse_http_request("THIS IS GARBAGE\r\n\r\n")

    def test_too_few_tokens_request_line(self):
        with pytest.raises(ParsingError):
            parse_http_request("GET\r\n\r\n")

    def test_header_without_colon(self):
        with pytest.raises(ParsingError):
            parse_http_request("GET / HTTP/1.0\r\nNoColonHere\r\n\r\n")

    def test_non_numeric_content_length(self):
        with pytest.raises(ParsingError):
            parse_http_request("POST / HTTP/1.0\r\nContent-Length: abc\r\n\r\n")

    def test_too_many_tokens_request_line(self):
        with pytest.raises(ParsingError):
            parse_http_request("GET / HTTP/1.1 extra\r\n\r\n")

    def test_bad_http_version(self):
        with pytest.raises(ParsingError):
            parse_http_request("GET / HTTP/9.9\r\n\r\n")


class TestRequestParserPipelining:
    def test_two_requests_one_buffer(self):
        requests = parse_http_request("GET /a HTTP/1.1\r\nHost: a\r\n\r\n"
                                      "GET /b HTTP/1.1\r\nHost: b\r\n\r\n")
        assert len(requests) == 2
        assert requests[0].path == "/a"
        assert requests[0].headers["host"] == "a"
        assert requests[1].path == "/b"
        assert requests[1].headers["host"] == "b"

    def test_trailing_partial_request_ignored(self):
        requests = parse_http_request("GET /a HTTP/1.1\r\nHost: a\r\n\r\nGET /b HT")
        assert len(requests) == 1
        assert requests[0].path == "/a"
        assert requests[0].headers["host"] == "a"

    def test_keep_alive_body_boundary(self):
        requests = parse_http_request(
            "POST /submit HTTP/1.1\r\nHost: a\r\nContent-Length: 5\r\n\r\n"
            "hello"
            "GET /next HTTP/1.1\r\nHost: a\r\n\r\n")
        assert len(requests) == 2
        assert requests[0].content == "hello"
        assert requests[1].path == "/next"
        assert requests[1].headers["host"] == "a"

