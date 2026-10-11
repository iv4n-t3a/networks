import pytest

from request import HttpRequestParser, ParsingError


@pytest.fixture
def parser() -> HttpRequestParser:
    return HttpRequestParser()


class TestRequestParserHttp09:
    def test_minimal(self, parser):
        [request] = parser.read("GET /path\r\n")
        assert request.http_version == "0.9"
        assert request.http_method == "GET"
        assert request.target == "/path"
        assert request.path == "/path"
        assert request.query_params == {}
        assert request.headers == {}
        assert request.content == ""

    def test_root_path(self, parser):
        [request] = parser.read("GET /\r\n")
        assert request.target == "/"
        assert request.path == "/"

    def test_query_params(self, parser):
        [request] = parser.read("GET /search?q=hello%20world&flag\r\n")
        assert request.path == "/search"
        assert len(request.query_params) == 2
        assert request.query_params["q"] == "hello world"
        assert request.query_params["flag"] == ""

    def test_unterminated_request_line_incomplete(self, parser):
        assert parser.read("GET /path") == []

    def test_split_across_reads(self, parser):
        assert parser.read("GET /pa") == []
        [request] = parser.read("th\r\n")
        assert request.path == "/path"

    def test_byte_by_byte(self, parser):
        data = "GET /path\r\n"
        for i in range(len(data) - 1):
            assert parser.read(data[i]) == []
        [request] = parser.read(data[-1])
        assert request.path == "/path"


class TestRequestParserHttp11:
    def test_minimal_with_host(self, parser):
        [request] = parser.read("GET / HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.http_version == "1.1"
        assert request.headers["host"] == "a"

    def test_missing_host_lenient(self, parser):
        [request] = parser.read("GET / HTTP/1.1\r\n\r\n")
        assert request.http_version == "1.1"
        assert request.headers == {}

    def test_absolute_form_target(self, parser):
        [request] = parser.read(
            "GET http://example.com/a/b?x=1 HTTP/1.1\r\nHost: example.com\r\n\r\n")
        assert request.target == "http://example.com/a/b?x=1"
        assert request.path == "/a/b"
        assert len(request.query_params) == 1
        assert request.query_params["x"] == "1"

    def test_url_encoded_query(self, parser):
        [request] = parser.read("GET /search?q=hello%20world HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.path == "/search"
        assert len(request.query_params) == 1
        assert request.query_params["q"] == "hello world"

    def test_key_only_query_param(self, parser):
        [request] = parser.read("GET /?flag HTTP/1.1\r\nHost: a\r\n\r\n")
        assert len(request.query_params) == 1
        assert request.query_params["flag"] == ""

    def test_duplicate_query_keys(self, parser):
        [request] = parser.read("GET /?a=1&a=2 HTTP/1.1\r\nHost: a\r\n\r\n")
        assert len(request.query_params) == 1
        assert request.query_params["a"] == "2"

    def test_empty_query_value(self, parser):
        [request] = parser.read("GET /?a= HTTP/1.1\r\nHost: a\r\n\r\n")
        assert len(request.query_params) == 1
        assert request.query_params["a"] == ""

    def test_plus_sign_not_decoded(self, parser):
        [request] = parser.read("GET /?q=a+b HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.query_params["q"] == "a+b"

    def test_extra_spaces_in_request_line(self, parser):
        [request] = parser.read("GET  /  HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.http_version == "1.1"
        assert request.path == "/"

    def test_asterisk_form_target(self, parser):
        [request] = parser.read("OPTIONS * HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.http_method == "OPTIONS"
        assert request.target == "*"


CHUNKED_HEADER = "POST / HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: chunked\r\n\r\n"

class TestRequestParserChunked:
    def test_single_chunk(self, parser):
        [request] = parser.read(CHUNKED_HEADER + "5\r\nhello\r\n0\r\n\r\n")
        assert request.content == "hello"

    def test_multiple_chunks(self, parser):
        [request] = parser.read(CHUNKED_HEADER + "4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n")
        assert request.content == "Wikipedia"

    def test_chunk_extensions(self, parser):
        [request] = parser.read(CHUNKED_HEADER + "5;ext=1;foo=bar\r\nhello\r\n0\r\n\r\n")
        assert request.content == "hello"

    def test_split_across_reads(self, parser):
        data = CHUNKED_HEADER + "5\r\nhello\r\n0\r\n\r\n"
        assert parser.read(data[:-6]) == []
        assert parser.read(data[-6:-1]) == []
        [request] = parser.read(data[-1])
        assert request.content == "hello"

    def test_uppercase_hex_chunk_size(self, parser):
        [request] = parser.read(CHUNKED_HEADER + "A\r\n0123456789\r\n0\r\n\r\n")
        assert request.content == "0123456789"

    def test_missing_terminating_chunk(self, parser):
        assert parser.read(CHUNKED_HEADER + "4\r\ntest\r\n") == []

    def test_non_numeric_chunk_size(self, parser):
        with pytest.raises(ParsingError):
            parser.read(CHUNKED_HEADER + "zz\r\nabc\r\n0\r\n\r\n")


class TestRequestParserErrors:
    def test_empty_read_is_no_op(self, parser):
        assert parser.read("") == []

    def test_garbage_request_line(self, parser):
        with pytest.raises(ParsingError):
            parser.read("THIS IS GARBAGE\r\n\r\n")

    def test_too_few_tokens_request_line(self, parser):
        with pytest.raises(ParsingError):
            parser.read("GET\r\n\r\n")

    def test_header_without_colon(self, parser):
        with pytest.raises(ParsingError):
            parser.read("GET / HTTP/1.0\r\nNoColonHere\r\n\r\n")

    def test_non_numeric_content_length(self, parser):
        with pytest.raises(ParsingError):
            parser.read("POST / HTTP/1.0\r\nContent-Length: abc\r\n\r\n")

    def test_too_many_tokens_request_line(self, parser):
        with pytest.raises(ParsingError):
            parser.read("GET / HTTP/1.1 extra\r\n\r\n")

    def test_bad_http_version(self, parser):
        with pytest.raises(ParsingError):
            parser.read("GET / HTTP/9.9\r\n\r\n")


class TestRequestParserPipelining:
    def test_two_requests_one_buffer(self, parser):
        requests = parser.read("GET /a HTTP/1.1\r\nHost: a\r\n\r\n"
                                      "GET /b HTTP/1.1\r\nHost: b\r\n\r\n")
        assert len(requests) == 2
        assert requests[0].path == "/a"
        assert requests[0].headers["host"] == "a"
        assert requests[1].path == "/b"
        assert requests[1].headers["host"] == "b"

    def test_trailing_partial_request_ignored(self, parser):
        requests = parser.read("GET /a HTTP/1.1\r\nHost: a\r\n\r\nGET /b HT")
        assert len(requests) == 1
        assert requests[0].path == "/a"
        assert requests[0].headers["host"] == "a"

    def test_keep_alive_body_boundary(self, parser):
        requests = parser.read(
            "POST /submit HTTP/1.1\r\nHost: a\r\nContent-Length: 5\r\n\r\n"
            "hello"
            "GET /next HTTP/1.1\r\nHost: a\r\n\r\n")
        assert len(requests) == 2
        assert requests[0].content == "hello"
        assert requests[1].path == "/next"
        assert requests[1].headers["host"] == "a"


class TestHttpRequestParserBuffering:
    def test_request_line_split_across_reads(self, parser):
        assert parser.read("GET /pa") == []
        [request] = parser.read("th HTTP/1.1\r\nHost: a\r\n\r\n")
        assert request.path == "/path"

    def test_headers_split_across_reads(self, parser):
        assert parser.read("GET / HTTP/1.1\r\nHost: a\r\nX-Cus") == []
        [request] = parser.read("tom: v\r\n\r\n")
        assert request.headers["x-custom"] == "v"

    def test_content_length_body_split_across_reads(self, parser):
        assert parser.read(
            "POST /submit HTTP/1.1\r\nHost: a\r\nContent-Length: 5\r\n\r\nhe") == []
        [request] = parser.read("llo")
        assert request.content == "hello"

    def test_pipelined_requests_split_across_reads(self, parser):
        [first] = parser.read("GET /a HTTP/1.1\r\nHost: a\r\n\r\nGET /b HT")
        assert first.path == "/a"
        [second] = parser.read("TP/1.1\r\nHost: b\r\n\r\n")
        assert second.path == "/b"

    def test_body_boundary_between_buffered_requests(self, parser):
        assert parser.read(
            "POST /a HTTP/1.1\r\nHost: a\r\nContent-Length: 5\r\n\r\nhe") == []
        [first, second] = parser.read("lloGET /b HTTP/1.1\r\nHost: b\r\n\r\n")
        assert first.content == "hello"
        assert second.path == "/b"

    def test_all_buffered_requests_returned_together(self, parser):
        [first] = parser.read("GET /a HTTP/1.1\r\nHost: a\r\n\r\nGET /b HT")
        assert first.path == "/a"
        requests = parser.read(
            "TP/1.1\r\nHost: b\r\n\r\nGET /c HTTP/1.1\r\nHost: c\r\n\r\n")
        assert [request.path for request in requests] == ["/b", "/c"]

