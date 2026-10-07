from dataclasses import dataclass, field


_STATUS_TEXTS = {
    200: "OK",
    201: "Created",
    204: "No Content",
    301: "Moved Permanently",
    302: "Found",
    400: "Bad Request",
    403: "Forbidden",
    404: "Not Found",
    405: "Method Not Allowed",
    500: "Internal Server Error",
    501: "Not Implemented",
    503: "Service Unavailable",
}

@dataclass
class Response:
    status_code: int
    headers: dict[str, str] = field(default_factory=dict)
    content: str = ""


def encode_response(resp: Response, version: str) -> str:
    if version == '0.9':
        return resp.content
    elif version == '1.0' or version == '1.1':
        status_text = _STATUS_TEXTS.get(resp.status_code)

        if len(resp.headers) != 0:
            headers = '\r\n'.join([f'{key}: {resp.headers[key]}' for key in resp.headers])
            return f'HTTP/{version} {resp.status_code} {status_text}\r\n{headers}\r\n\r\n{resp.content}'
        return f'HTTP/{version} {resp.status_code} {status_text}\r\n\r\n{resp.content}'
    else:
        raise ValueError('Unknown http version')
