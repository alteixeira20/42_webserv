# Runtime Foundation

This document describes the runtime foundation integrated from
`feature/listener-socket`. It documents the modules that exist now, their
boundaries with the config layer, and the runtime work that is still pending.

It does not claim that the server runtime is complete.

## Scope And Ownership

Status: Implemented

- Listener socket setup foundation.
- `poll()`-based event readiness wrapper.
- Client connection state holder.
- Client accept/store foundation.
- Runtime tests for the foundation modules.

Status: Not responsible for

- Full HTTP request parsing.
- HTTP response generation.
- Route resolution.
- Filesystem serving.
- Upload behavior.
- CGI execution.
- Complete production socket read/write handling.

## Config Boundary

Status: Implemented

The runtime consumes typed config objects from the existing config foundation.
The partner branch originally introduced a separate `ListenerConfig` model, but
that duplicate endpoint type was removed during integration.

Current endpoint flow:

```text
ConfigParser -> Config -> Config::getUniqueListens()
             -> std::vector<ListenConfig>
             -> ListenerManager::openAll()
```

Important invariants:

- `Config` owns parsed `ServerConfig` data.
- `ServerConfig` owns one or more `ListenConfig` endpoints.
- `Config::getUniqueListens()` deduplicates configured listen endpoints.
- Runtime binds one socket per unique `ListenConfig`.
- Runtime must not parse config strings directly.
- Runtime receives endpoint objects, not raw parser tokens or directive text.

Status: Implemented

`src/main.cpp` now relies on semantic config parsing. `ConfigParser` populates
`ServerConfig` and `ListenConfig` from real `listen` directives, and
`Config::getUniqueListens()` supplies the listener endpoints. The previous
`0.0.0.0:8080` temporary fallback was removed.

Empty or invalid config is a startup `ConfigException`, not a runtime fallback.

## ListenerManager

Status: Implemented

`ListenerManager` owns listener socket setup and cleanup.

What it does:

- Receives `std::vector<ListenConfig>` through `openAll()`.
- Opens one listener socket per `ListenConfig`.
- Resolves host and port with `getaddrinfo()`.
- Creates TCP sockets.
- Sets `SO_REUSEADDR`.
- Binds each socket to its endpoint.
- Calls `listen()`.
- Sets listener fds to non-blocking mode.
- Stores listener fd plus the `ListenConfig` that produced it.
- Closes listener fds in `closeAll()` and the destructor.
- Cleans up already-opened listeners if opening a later endpoint fails.

What it does not do:

- It does not parse config files or config strings.
- It does not deduplicate endpoints.
- It does not accept clients.
- It does not run the main event loop.
- It does not read requests or write responses.

## EventLoop

Status: Implemented

`EventLoop` is a small wrapper around `poll()`.

What it does:

- Registers listener fds.
- Registers client fds through `ClientConnection`.
- Registers CGI pipe fds with explicit read/write interest.
- Builds `pollfd` entries from current watch state.
- Calls `poll()`.
- Reports readiness as `EventLoopEvent` objects.
- Reports readable, writable, closed, and error flags.
- Supports listener, client, and CGI pipe watch types.
- Can monitor read and write interest during the same `poll()` call.

What it does not do:

- It does not accept clients.
- It does not call `read()`, `recv()`, `write()`, or `send()`.
- It does not close sockets.
- It does not own client lifetime.
- It does not execute CGI.

## ClientConnection

Status: Implemented

`ClientConnection` represents one accepted client fd and its runtime state.

What it stores:

- Client fd.
- Listener fd that accepted the client.
- Connection state.
- Read buffer.
- Write buffer.
- Close reason.
- Last activity timestamp.

What it exposes:

- `wantsRead()` for polling read interest.
- `wantsWrite()` for polling write interest.
- Buffer append/consume helpers.
- State transition helpers.
- Close reason recording.

What it does not do:

- It does not parse HTTP.
- It does not generate responses.
- It does not perform socket I/O.
- It does not decide route behavior.
- It does not execute CGI.

## ClientManager

Status: Implemented

`ClientManager` owns accepted client storage and accept-time setup.

What it does:

- Accepts clients from ready listener events.
- Requires the listener event to be readable before accepting.
- Sets accepted client sockets to non-blocking mode.
- Stores `ClientConnection` objects.
- Registers accepted clients with `EventLoop`.
- Closes all stored client fds in `closeAll()` and the destructor.

What it does not do:

- It does not implement production client reads.
- It does not implement production client writes.
- It does not remove individual clients on I/O error or disconnect yet.
- It does not parse HTTP requests.
- It does not build HTTP responses.

## Current Limitations

Status: Temporary

- `main` opens listeners and then exits.
- There is no persistent main loop yet.
- There is no HTTP request read path.
- There is no response write path.
- There is no CGI pipe execution.
- There is no per-client I/O error or disconnect cleanup.
- Runtime currently depends on parsed listen endpoints; there is no temporary
  fallback endpoint.

## Evaluation-Critical Constraints Still Pending

Status: Planned

- Implement the full main `poll()`/`select()`/equivalent loop.
- Monitor read and write readiness simultaneously in the main loop.
- Never read or write sockets without readiness.
- Never read or write CGI pipes without readiness.
- Perform at most one read or one write per client per poll cycle.
- Check `read()`, `recv()`, `write()`, and `send()` return values correctly.
- Do not use `errno` after `read()`, `recv()`, `write()`, or `send()` to decide
  normal behavior.
- Close and remove clients cleanly on disconnect.
- Close and remove clients cleanly on socket I/O error.
- Enforce CGI pipe readiness discipline.

## Next Runtime Tasks

Status: Planned

- Wire listener fds into a persistent `EventLoop`.
- Accept clients continuously from readable listener events.
- Implement guarded client reads.
- Implement guarded client writes.
- Implement per-client cleanup and removal.
- Integrate the HTTP parser.
- Integrate response building.
- Later integrate CGI pipes with readiness-aware reads and writes.

