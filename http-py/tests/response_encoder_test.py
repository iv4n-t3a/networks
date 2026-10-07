import pytest

from response import Response, encode_response


class TestEncodeResponseHttp09:
    def test_body_only(self):
        resp = Response(status_code=200, content="hello")
        assert encode_response(resp, "0.9") == "hello"

    def test_headers_and_status_ignored(self):
        resp = Response(status_code=404,
                        headers={"X-A": "b"},
                        content="missing")
        assert encode_response(resp, "0.9") == "missing"

    def test_empty_body(self):
        resp = Response(status_code=200)
        assert encode_response(resp, "0.9") == ""


class TestEncodeResponseHttp10:

    def test_minimal(self):
        resp = Response(status_code=200, content="hello")
        assert encode_response(resp, "1.0") == (
            "HTTP/1.0 200 OK\r\n"
            "\r\n"
            "hello")

    def test_with_headers(self):
        resp = Response(status_code=200,
                        headers={"Content-Type": "text/html",
                                 "Content-Length": "5"},
                        content="hello")
        assert encode_response(resp, "1.0") == (
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "hello")

    def test_empty_body(self):
        resp = Response(status_code=204)
        assert encode_response(resp, "1.0") == "HTTP/1.0 204 No Content\r\n\r\n"


class TestEncodeResponseHttp11:
    def test_minimal(self):
        resp = Response(status_code=200, content="hello")
        assert encode_response(resp, "1.1") == (
            "HTTP/1.1 200 OK\r\n"
            "\r\n"
            "hello")

    def test_with_headers(self):
        resp = Response(status_code=200,
                        headers={"Content-Type": "text/plain"},
                        content="hello")
        assert encode_response(resp, "1.1") == (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "hello")


class TestEncodeResponseErrors:
    def test_unknown_version(self):
        resp = Response(status_code=200, content="hello")
        with pytest.raises(ValueError):
            encode_response(resp, "2.0")
