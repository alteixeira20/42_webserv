# Runtime Tests

Status: Implemented

Runtime foundation tests live in this directory and are built by the existing
`Makefile` targets:

- `tests/runtime/test_listener_manager.cpp`
- `tests/runtime/test_event_loop.cpp`
- `tests/runtime/test_client_connection.cpp`
- `tests/runtime/test_client_manager.cpp`

Run them through:

```sh
python3 tests/run.py runtime
```

or directly through:

```sh
make test_runtime
```

Current scope:

- listener socket setup and cleanup;
- `poll()` readiness reporting;
- client connection state;
- accepting ready clients and registering them with `EventLoop`.

Not implemented here:

- production client read/write handling;
- persistent main loop;
- HTTP parser integration;
- CGI pipe execution.
