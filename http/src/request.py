import urllib.parse
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Dict


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


def parse_http_request(data: str) -> list[Request]:
    data = data.replace('\r\n', '\n')

    lines = data.split('\n')

    if len(lines) == 1:
        return []

    request_line = lines[0]

    # No http version
    if len(request_line.split()) == 2:
        return parse_http_09_request(data)

    http_version = request_line.split()[2].upper()

    if http_version == 'HTTP/1.0':
        return parse_http_10_request(data)
    elif http_version == 'HTTP/1.1':
        return parse_http_11_request(data)
    else:
        raise ParsingError("Unknown http version")


def parse_http_09_request(data: str) -> list[Request]:
    http_method = data.split()[0].upper()

    if http_method != 'GET':
        raise ParsingError("HTTP/0.9 is GET-only")

    target = data.split()[1]

    return [Request(
        http_version='0.9',
        http_method='GET',
        path=extract_path(target),
        query_params=extract_query_params(target),
        target=target
    )]


def parse_http_10_request(data: str) -> list[Request]:
    # Request + headers + content
    if len(data.split('\n\n')) < 2:
        return []

    http_method = data.split()[0].upper()
    target = data.split()[1]
    headers = extract_headers(data)
    content = data.split('\n\n')[1]

    if 'content-length' in headers and int(headers['content-length']) != len(content):
        return []

    return [Request(
        http_version='1.0',
        http_method=http_method,
        path=extract_path(target),
        query_params=extract_query_params(target),
        target=target,
        headers=headers,
        content=content,
    )]

def parse_http_11_request(data: str) -> list[Request]:
    # Request + headers + content
    if len(data.split('\n\n')) < 2:
        return []

    http_method = data.split()[0].upper()
    target = data.split()[1]
    headers = extract_headers(data)
    content = data.split('\n\n')[1]

    if 'content-length' in headers and int(headers['content-length']) != len(content):
        return []

    return [Request(
        http_version='1.1',
        http_method=http_method,
        path=extract_path(target),
        query_params=extract_query_params(target),
        target=target,
        headers=headers,
        content=content,
    )]

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

    for header in data.split("\n")[1:]:
        if header == '':
            return res

        key, value = header.split(':', 1)

        key = key.lower().strip()
        value = value.strip()

        if key in res:
            res[key] += value
        else:
            res[key] = value

    return res
