import urllib.parse
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Dict, Optional


class ParsingError(Exception):
    pass


@dataclass
class Request:
    http_version: str
    http_method: str
    path: str
    target: str
    query_params: dict[str, str] = field(default_factory=dict)
    headers: dict[str, str] = field(default_factory=dict)
    content: str = ""


class HttpRequestParser:
    def __init__(self) -> None:
        self._buffer = ""

    def read(self, data: str) -> list[Request]:
        self._buffer += data

        requests = []

        while request := self._read_http_request():
            requests.append(request)

        return requests

    def _read_http_request(self) -> Optional[Request]:
        lines = self._buffer.split('\r\n')

        if len(lines) == 1:
            return None

        request_line = lines[0]

        if len(request_line.split()) == 2:
            return self._read_http_09_request()
        elif len(request_line.split()) != 3:
            raise ParsingError("Can't parse request line, expected 2 or 3 tokens")

        http_version = request_line.split()[2].upper()

        if http_version in ('HTTP/1.0', 'HTTP/1.1'):
            return self._read_http_11_request()
        else:
            raise ParsingError("Unknown http version")

    def _read_http_09_request(self) -> Optional[Request]:
        http_method = self._buffer.split()[0].upper()

        if http_method != 'GET':
            raise ParsingError("HTTP/0.9 is GET-only")

        target = self._buffer.split()[1]

        self._buffer = ""

        return Request(
            http_version='0.9',
            http_method='GET',
            path=extract_path(target),
            query_params=extract_query_params(target),
            target=target
        )

    def _read_http_11_request(self) -> Optional[Request]:
        if len(self._buffer.split('\r\n\r\n')) < 2:
            return None

        http_method = self._buffer.split()[0].upper()
        target = self._buffer.split()[1]
        http_version = self._buffer.split()[2].upper()
        headers = extract_headers(self._buffer)
        content = self._read_http_11_content(headers)

        if content is None:
            return None

        self._buffer = self._buffer.lstrip()

        return Request(
            http_version=http_version.split('/')[1],
            http_method=http_method,
            path=extract_path(target),
            query_params=extract_query_params(target),
            target=target,
            headers=headers,
            content=content,
        )

    def _read_http_11_content(self, headers: Dict[str, str]) -> Optional[str]:
        if 'content-length' in headers:
            return self._read_http_11_by_content_length(headers)
        elif headers.get('transfer-encoding') == 'chunked':
            return self._read_http_11_chunked_content()
        else:
            _, self._buffer = self._buffer.split('\r\n\r\n', 1)
            return ''

    def _read_http_11_by_content_length(self, headers: Dict[str, str]) -> Optional[str]:
        _, content = self._buffer.split('\r\n\r\n', 1)

        try:
            content_lenght = int(headers['content-length'])
        except:
            raise ParsingError('Failed to parse content length header')

        if len(content) < content_lenght:
            return None
        content, self._buffer = content[:content_lenght], content[content_lenght:]
        return content


    def _read_http_11_chunked_content(self) -> Optional[str]:
        _, body = self._buffer.split('\r\n\r\n', 1)

        chunks = []
        while True:
            header_end = body.find('\r\n')
            if header_end == -1:
                return None

            try:
                chunk_size = int(body[:header_end].split(';')[0].strip(), 16)
            except ValueError:
                raise ParsingError("Invalid chunk size")

            if chunk_size == 0:
                # Last chunk, needs its CRLF plus the terminating CRLF
                if len(body) < header_end + 4:
                    return None
                self._buffer = body[header_end + 4:]
                return ''.join(chunks)

            data_start = header_end + 2
            data_end = data_start + chunk_size
            if len(body) < data_end + 2:
                return None
            chunks.append(body[data_start:data_end])
            body = body[data_end + 2:]


def extract_path(target: str) -> str:
    if '?' in target:
        target = target.split('?')[0]

    scheme_end = target.find('://')
    if scheme_end != -1:
        slash = target.find('/', scheme_end + 3)
        if slash == -1:
            return '/'
        return target[slash:]

    return target

def extract_query_params(target: str) -> Dict[str, str]:
    if '?' not in target:
        return {}

    query_params_str=target.split('?')[1]
    query_params_pairs = query_params_str.split('&')
    res = dict()

    for query_param_pair in query_params_pairs:
        if '=' not in query_param_pair:
            res[query_param_pair] = ''
            continue

        key, value = query_param_pair.split('=')
        res[key] = urllib.parse.unquote(value)

    return res

def extract_headers(data: str) -> Dict[str, str]:
    res = dict()

    for header in data.split("\r\n")[1:]:
        if header == '':
            return res

        if ':' not in header:
            raise ParsingError("Header without colon")

        key, value = header.split(':', 1)

        key = key.lower().strip()
        value = value.strip()

        if key in res:
            res[key] += value
        else:
            res[key] = value

    return res
