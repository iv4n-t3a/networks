import json
import uuid
from datetime import datetime, timezone

from request import Request
from response import Response


def _now() -> str:
    return datetime.now(timezone.utc).isoformat()


def _json_response(status_code: int, payload) -> Response:
    return Response(
        status_code=status_code,
        headers={"Content-Type": "application/json"},
        content=json.dumps(payload),
    )


def _parse_json_body(request: Request) -> dict | None:
    try:
        data = json.loads(request.content)
    except json.JSONDecodeError:
        return None

    if not isinstance(data, dict):
        return None

    return data


class NotesStore:
    def __init__(self) -> None:
        self._notes: dict[str, dict] = {}

    def create(self, title: str, content: str) -> dict:
        now = _now()
        note = {
            "id": uuid.uuid4().hex,
            "title": title,
            "content": content,
            "created_at": now,
            "updated_at": now,
        }
        self._notes[note["id"]] = note
        return note

    def list(self) -> list[dict]:
        return list(self._notes.values())

    def update(self, note_id: str, data: dict) -> dict | None:
        note = self._notes.get(note_id)
        if note is None:
            return None

        for field in ("title", "content"):
            if field in data:
                note[field] = data[field]
        note["updated_at"] = _now()
        return note

    def delete(self, note_id: str) -> bool:
        return self._notes.pop(note_id, None) is not None


class NotesApp:
    def __init__(self) -> None:
        self._store = NotesStore()

    def handle(self, request: Request) -> Response:
        if request.path == "/notes":
            if request.http_method == "POST":
                return self._create(request)
            if request.http_method == "GET":
                return self._list()
            return _json_response(405, {"error": "Method Not Allowed"})

        path_parts = request.path.strip("/").split("/")
        if len(path_parts) == 2 and path_parts[0] == "notes":
            note_id = path_parts[1]
            if request.http_method == "PUT":
                return self._update(request, note_id)
            if request.http_method == "DELETE":
                return self._delete(note_id)
            return _json_response(405, {"error": "Method Not Allowed"})

        return _json_response(404, {"error": "Not Found"})

    def _create(self, request: Request) -> Response:
        data = _parse_json_body(request)
        if data is None:
            return _json_response(400, {"error": "Invalid JSON body"})

        title = data.get("title")
        if not isinstance(title, str) or not title:
            return _json_response(400, {"error": "title is required"})

        content = data.get("content", "")
        if not isinstance(content, str):
            return _json_response(400, {"error": "content must be a string"})

        return _json_response(201, self._store.create(title, content))

    def _list(self) -> Response:
        return _json_response(200, {"notes": self._store.list()})

    def _update(self, request: Request, note_id: str) -> Response:
        data = _parse_json_body(request)
        if data is None:
            return _json_response(400, {"error": "Invalid JSON body"})

        for field in ("title", "content"):
            if field in data and not isinstance(data[field], str):
                return _json_response(400, {"error": f"{field} must be a string"})

        note = self._store.update(note_id, data)
        if note is None:
            return _json_response(404, {"error": "Note not found"})

        return _json_response(200, note)

    def _delete(self, note_id: str) -> Response:
        if not self._store.delete(note_id):
            return _json_response(404, {"error": "Note not found"})

        return Response(status_code=204)
