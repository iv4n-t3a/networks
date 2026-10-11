import asyncio
import threading

import pytest
import requests

from server import Server


@pytest.fixture
def server():
    loop = asyncio.new_event_loop()
    thread = threading.Thread(target=loop.run_forever, daemon=True)
    thread.start()

    server = Server(0)
    run_future = asyncio.run_coroutine_threadsafe(server.run(), loop)
    asyncio.run_coroutine_threadsafe(server.wait_until_started(), loop).result(timeout=5)

    yield server

    run_future.cancel()
    asyncio.run_coroutine_threadsafe(server.wait_until_stopped(), loop).result(timeout=5)
    loop.call_soon_threadsafe(loop.stop)
    thread.join(timeout=5)


@pytest.fixture
def url(server) -> str:
    return f"http://127.0.0.1:{server.port}"


class TestChunkedEncoding:
    def test_many_chunks_larger_than_read_buffer(self, url):
        chunk = "hello " * 8192

        def body():
            yield '{"title": "big", "content": "'
            for _ in range(6):
                yield chunk
            yield '"}'

        response = requests.post(f"{url}/notes", data=body())

        assert response.status_code == 201
        note = response.json()
        assert note["title"] == "big"
        assert note["content"] == chunk * 6


class TestKeepAlive:
    def test_connection_reused_across_requests(self, server, url):
        session = requests.Session()

        response = session.post(f"{url}/notes", json={"title": "first"})
        assert response.status_code == 201

        response = session.get(f"{url}/notes")
        assert response.status_code == 200
        assert [note["title"] for note in response.json()["notes"]] == ["first"]

        assert server.connections == 1

    def test_chunked_requests_reuse_connection(self, server, url):
        session = requests.Session()

        for i in range(3):
            def body(i=i):
                yield f'{{"title": "note {i}"}}'

            response = session.post(f"{url}/notes", data=body())
            assert response.status_code == 201

        response = session.get(f"{url}/notes")
        assert len(response.json()["notes"]) == 3
        assert server.connections == 1

