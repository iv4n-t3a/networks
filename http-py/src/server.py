import asyncio
import socket

from request import ParsingError, Request, parse_http_request
from response import Response, encode_response

_READ_CHUNK_SIZE = 64 * 1024


def _make_response(request: Request) -> Response:
    """Builds the echo response for a parsed request.

    The status comes from the x-status header (default 200) and response
    headers mirror the request's x-header-... headers.
    """
    status_code = 200
    if "x-status" in request.headers:
        try:
            status_code = int(request.headers["x-status"])
        except ValueError:
            pass

    response_headers = {
        name[len("x-header-"):]: value
        for name, value in request.headers.items()
        if name.startswith("x-header-")
    }

    return Response(
        status_code=status_code,
        headers=response_headers,
        content=str(request),
    )


class Server:
    def __init__(self, port: int, ipv6: bool = False) -> None:
        self._port = port
        self._ipv6 = ipv6

    async def run(self) -> None:
        family = socket.AF_INET6 if self._ipv6 else socket.AF_INET
        server = await asyncio.start_server(
            self._process, "", self._port, family=family)

        async with server:
            await server.serve_forever()

    async def _process(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        try:
            buffer = b""

            while True:
                chunk = await reader.read(_READ_CHUNK_SIZE)
                if not chunk:
                    return

                buffer += chunk

                try:
                    requests = parse_http_request(buffer.decode())
                except ParsingError:
                    return

                if not requests:
                    continue

                for request in requests:
                    response = _make_response(request)
                    writer.write(
                        encode_response(response, request.http_version).encode())
                await writer.drain()

                buffer = b""
                break
        except (ConnectionError, asyncio.IncompleteReadError):
            pass
        finally:
            writer.close()
            await writer.wait_closed()
