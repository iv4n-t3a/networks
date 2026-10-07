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

Result is a serialized parsed response object. Response status comes from x-status header. Status text is a pre-mapped value. x-header headers are proxied to response headers.
