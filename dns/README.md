## DNS-server

### Build and run

build:
```bash
  cmake -S . -B build
  make -C build
```

Run:
```bash
./build/dns-server
```

Check:
```bash
dig @localhost -p 1234 example.com
```

### Configure

To configure server edit file `src/config.hpp`. Then recompile the project, to apply changes.

### Tests

Run:
```bash
cmake -S . -B build
cmake --build build --target dns-server-tests
./build/dns-server-tests
```

Or through CTest:
```bash
ctest --test-dir build --output-on-failure
```

To skip building the tests, configure with `-DBUILD_TESTING=OFF`.
