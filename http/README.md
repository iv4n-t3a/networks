## HTTP-server

### Features

- HTTP/1.1

**TODO**:

- keep-alive
- chunk-encoding
- http/2, http/3

### Run

No build step needed:

```bash
python3 src/main.py
```

Options:

```bash
python3 src/main.py --port 8080 --ipv6
```

### Tests

```bash
pip install -r requirements-dev.txt
pytest
```

### Usage

REST API for in-memory notes (no persistence — all data is lost when the server stops).
The OpenAPI spec lives in [openapi/openapi.yaml](openapi/openapi.yaml).

| Method | Path           | Description   |
|--------|----------------|---------------|
| POST   | /notes         | Create a note |
| GET    | /notes         | List notes    |
| PUT    | /notes/{id}    | Update a note |
| DELETE | /notes/{id}    | Delete a note |

```bash
# Create
curl -X POST localhost/notes \
  -H 'Content-Type: application/json' \
  -d '{"title": "hello", "content": "world"}'

# List
curl localhost/notes

# Update (fields omitted are left unchanged)
curl -X PUT localhost/notes/<id> \
  -H 'Content-Type: application/json' \
  -d '{"title": "hello 2"}'

# Delete
curl -X DELETE localhost/notes/<id>
```
