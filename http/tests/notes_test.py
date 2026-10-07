import json

from notes import NotesApp, NotesStore
from request import Request


def make_request(method: str, path: str, content: str = "") -> Request:
    return Request(
        http_version="1.1",
        http_method=method,
        path=path,
        target=path,
        content=content,
    )


def make_json(method: str, path: str, payload) -> Request:
    return make_request(method, path, json.dumps(payload))


class TestCreateNote:
    def test_creates_note(self):
        app = NotesApp()
        response = app.handle(make_json("POST", "/notes", {
            "title": "hello",
            "content": "world",
        }))

        assert response.status_code == 201
        note = json.loads(response.content)
        assert note["title"] == "hello"
        assert note["content"] == "world"
        assert note["id"]
        assert note["created_at"] == note["updated_at"]

    def test_created_note_is_listed(self):
        app = NotesApp()
        app.handle(make_json("POST", "/notes", {"title": "hello"}))

        response = app.handle(make_request("GET", "/notes"))
        assert response.status_code == 200
        notes = json.loads(response.content)["notes"]
        assert len(notes) == 1
        assert notes[0]["title"] == "hello"
        assert notes[0]["content"] == ""

    def test_invalid_json(self):
        app = NotesApp()
        response = app.handle(make_request("POST", "/notes", "{oops"))
        assert response.status_code == 400

    def test_missing_title(self):
        app = NotesApp()
        response = app.handle(make_json("POST", "/notes", {"content": "x"}))
        assert response.status_code == 400

    def test_non_object_body(self):
        app = NotesApp()
        response = app.handle(make_request("POST", "/notes", "[1, 2]"))
        assert response.status_code == 400


class TestListNotes:
    def test_empty_store(self):
        app = NotesApp()
        response = app.handle(make_request("GET", "/notes"))
        assert response.status_code == 200
        assert json.loads(response.content) == {"notes": []}


class TestUpdateNote:
    def test_updates_fields(self):
        app = NotesApp()
        note = json.loads(
            app.handle(make_json("POST", "/notes", {"title": "a",
                                                    "content": "b"})).content)

        response = app.handle(make_json("PUT", f"/notes/{note['id']}", {
            "title": "new title",
        }))

        assert response.status_code == 200
        updated = json.loads(response.content)
        assert updated["title"] == "new title"
        assert updated["content"] == "b"
        assert updated["updated_at"] > note["updated_at"]

    def test_unknown_note(self):
        app = NotesApp()
        response = app.handle(make_json("PUT", "/notes/missing", {"title": "x"}))
        assert response.status_code == 404

    def test_invalid_field_type(self):
        app = NotesApp()
        note = json.loads(
            app.handle(make_json("POST", "/notes", {"title": "a"})).content)

        response = app.handle(make_json("PUT", f"/notes/{note['id']}", {
            "title": 42,
        }))
        assert response.status_code == 400


class TestDeleteNote:
    def test_deletes_note(self):
        app = NotesApp()
        note = json.loads(
            app.handle(make_json("POST", "/notes", {"title": "a"})).content)

        response = app.handle(make_request("DELETE", f"/notes/{note['id']}"))
        assert response.status_code == 204
        assert response.content == ""

        listed = json.loads(
            app.handle(make_request("GET", "/notes")).content)["notes"]
        assert listed == []

    def test_unknown_note(self):
        app = NotesApp()
        response = app.handle(make_request("DELETE", "/notes/missing"))
        assert response.status_code == 404


class TestRouting:
    def test_unknown_path(self):
        app = NotesApp()
        assert app.handle(make_request("GET", "/")).status_code == 404

    def test_method_not_allowed(self):
        app = NotesApp()
        assert app.handle(make_request("DELETE", "/notes")).status_code == 405
        assert app.handle(make_request("GET", "/notes/123")).status_code == 405


class TestNotesStore:
    def test_delete_returns_false_for_missing_note(self):
        store = NotesStore()
        assert store.delete("missing") is False
