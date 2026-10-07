import asyncio
import socket

from notes import NotesApp
from request import ParsingError, parse_http_request
from response import encode_response

_READ_CHUNK_SIZE = 64 * 1024


class Server:
    def __init__(self, port: int, ipv6: bool = False) -> None:
        self._port = port
        self._ipv6 = ipv6
        self._app = NotesApp()

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
                    response = self._app.handle(request)
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
