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
- Client cleanup for timeouts, closing connections, and full shutdown.
- Guarded client socket read/write helper.
- Dummy fixed-response runtime path.
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
- Stores the actual bound port reported by `getsockname()`, so port `0`
  listeners expose the kernel-assigned port.
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
- `isTimedOut()` based on last activity and a caller-provided timeout.

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
- Closes and removes timed-out clients.
- Removes clients already in the closing state.
- Closes all stored client fds in `closeAll()`, `closeAll(EventLoop&)`, and
  the destructor.

What it does not do:

- It does not parse HTTP requests.
- It does not build HTTP responses.

## ClientIo

Status: Implemented

`ClientIo` performs one guarded socket read or write for a ready client event.

What it does:

- Reads only when the event is a readable client event and the connection wants
  reads.
- Appends positive read bytes to the client read buffer.
- Marks EOF as a closing client with `client closed connection`.
- Marks non-`EAGAIN` read errors as `client read error`.
- Writes only when the event is a writable client event and the connection has
  buffered response data.
- Consumes only the bytes actually written.
- Marks non-`EAGAIN` write errors as `client write error`.

What it does not do:

- It does not parse request bytes.
- It does not decide when a request is complete.
- It does not generate a response.

## DummyResponseRuntime

Status: Implemented

`DummyResponseRuntime` is a temporary runtime path used to exercise listener
acceptance, client I/O, response buffering, and cleanup before the real HTTP
pipeline is integrated.

What it does:

- Polls the `EventLoop` once.
- Accepts ready listener events through `ClientManager`.
- Handles ready client events through `ClientIo`.
- Queues a minimal fixed `HTTP/1.1 200 OK` response after reading request bytes.
- Closes clients after the fixed response is sent.
- Removes closing clients after each pump.

What it does not do:

- It does not parse HTTP.
- It does not route requests.
- It does not serve files.
- It does not execute CGI.

## Current Limitations

Status: Temporary

- `main` opens listeners and then exits.
- There is no persistent main loop yet.
- There is a temporary dummy response path, not the real HTTP response pipeline.
- There is no CGI pipe execution.
- Runtime currently depends on parsed listen endpoints; there is no temporary
  fallback endpoint.

## Evaluation-Critical Constraints Still Pending

Status: Planned

- Implement the full main `poll()`/`select()`/equivalent loop.
- Monitor read and write readiness simultaneously in the main loop.
- Never read or write CGI pipes without readiness.
- Perform at most one read or one write per client per poll cycle.
- Integrate guarded client I/O into the full server loop.
- Enforce CGI pipe readiness discipline.

## Next Runtime Tasks

Status: Planned

- Wire listener fds into a persistent `EventLoop`.
- Accept clients continuously from readable listener events.
- Integrate the HTTP parser.
- Integrate response building.
- Later integrate CGI pipes with readiness-aware reads and writes.

