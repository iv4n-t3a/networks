import asyncio
import codecs
import socket

from notes import NotesApp
from request import HttpRequestParser, ParsingError, Request
from response import encode_response

_READ_CHUNK_SIZE = 64 * 1024


class Server:
    def __init__(self, port: int, ipv6: bool = False) -> None:
        self._port = port
        self._ipv6 = ipv6
        self._app = NotesApp()
        self._server: asyncio.Server | None = None
        self._started = asyncio.Event()
        self._stopped = asyncio.Event()
        self.connections = 0

    @property
    def port(self) -> int:
        assert self._server is not None, "server is not started"
        return self._server.sockets[0].getsockname()[1]

    async def wait_until_started(self) -> None:
        await self._started.wait()

    async def wait_until_stopped(self) -> None:
        await self._stopped.wait()

    async def run(self) -> None:
        family = socket.AF_INET6 if self._ipv6 else socket.AF_INET
        self._server = await asyncio.start_server(
            self._process, "", self._port, family=family)
        self._started.set()

        try:
            async with self._server:
                await self._server.serve_forever()
        finally:
            self._stopped.set()

    async def _process(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        self.connections += 1

        parser = HttpRequestParser()
        decoder = codecs.getincrementaldecoder("utf-8")()

        try:
            while True:
                chunk = await reader.read(_READ_CHUNK_SIZE)
                if not chunk:
                    return

                try:
                    requests = parser.read(decoder.decode(chunk))
                except (ParsingError, UnicodeDecodeError):
                    return

                if not requests:
                    continue

                for request in requests:
                    response = self._app.handle(request)
                    writer.write(
                        encode_response(response, request.http_version).encode())

                await writer.drain()

                if not all(_should_keep_alive(request) for request in requests):
                    return
        except (ConnectionError, asyncio.IncompleteReadError):
            pass
        finally:
            writer.close()
            await writer.wait_closed()


def _should_keep_alive(request: Request) -> bool:
    connection = request.headers.get("connection", "").lower()

    if request.http_version == "1.0":
        return connection == "keep-alive"
    if request.http_version == "1.1":
        return connection != "close"

    return False
