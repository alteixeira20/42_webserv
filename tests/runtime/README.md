# Runtime Tests

Status: Implemented

Runtime foundation tests live in this directory and are built by the existing
`Makefile` targets:

- `tests/runtime/test_listener_manager.cpp`
- `tests/runtime/test_event_loop.cpp`
- `tests/runtime/test_client_connection.cpp`
- `tests/runtime/test_client_cleanup.cpp`
- `tests/runtime/test_client_io.cpp`
- `tests/runtime/test_client_manager.cpp`
- `tests/runtime/test_dummy_response_runtime.cpp`
- `tests/runtime/test_runtime_loop.cpp`
- `tests/runtime/test_server_runtime.cpp`

Run them through:

```sh
python3 tests/run.py runtime
```

or directly through:

```sh
make test_runtime_internal
```

Current scope:

- listener socket setup and cleanup;
- actual listener bound port reporting for port `0`;
- `poll()` readiness reporting;
- client connection state;
- accepting ready clients and registering them with `EventLoop`;
- timeout, closing-state, and close-all client cleanup;
- guarded client read/write return handling;
- dummy fixed-response runtime path through accepted clients.
- one-cycle runtime event dispatch for listeners and clients;
- server runtime startup, counts, and shutdown cleanup.

Not implemented here:

- HTTP parser integration;
- CGI pipe execution.
