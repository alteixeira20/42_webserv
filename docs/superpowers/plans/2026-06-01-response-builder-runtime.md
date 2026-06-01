# Response Builder Runtime Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement WS-412 and WS-413: serialize simple HTTP responses and queue one when a `ClientConnection` reaches `PROCESSING`.

**Architecture:** Keep parsing, routing, and static file work out of scope. Add a small `HttpResponse` value type plus `ResponseBuilder`, then let `RuntimeLoop` turn `PROCESSING` clients with parsed headers into a serialized response, transition them to `WRITING_RESPONSE`, write through `ClientIo`, and close after the write buffer drains.

**Tech Stack:** C++98, existing `Makefile`, existing socket/poll runtime, existing plain C++ test binaries.

---

## Roadmap Context

- `WS-412` is marked `next: true`: create the HTTP response builder foundation.
- `WS-413` depends on `WS-412`: connect `PROCESSING` clients to response generation.
- `WS-410` is also `next: true` but is Alexandre's parser/body work; do not touch body parsing here.
- Do not implement `WS-405` error pages, `WS-403` routing, `WS-404` static files, CGI, uploads, DELETE, or chunked/body parsing in this branch.

## File Structure

- Create `include/http/HttpResponse.hpp`: response value object API.
- Create `src/http/HttpResponse.cpp`: status line, headers, body, and serialization.
- Create `include/http/ResponseBuilder.hpp`: minimal response factory API.
- Create `src/http/ResponseBuilder.cpp`: builds a fixed/simple close-after-response message from a parsed `HttpRequest`.
- Create `tests/http/test_http_response.cpp`: unit tests for response serialization and builder defaults.
- Modify `include/runtime/RuntimeLoop.hpp`: include `ResponseBuilder` and add private helpers for processing clients.
- Modify `src/runtime/RuntimeLoop.cpp`: queue responses for `PROCESSING` clients and close drained responses.
- Modify `Makefile`: compile new HTTP response sources and add the new HTTP test binary.
- Modify `tests/http/README.md`: document response-builder test coverage.
- Modify `tests/runtime/test_runtime_loop.cpp`: add runtime integration tests for `PROCESSING -> WRITING_RESPONSE -> CLOSING`.
- Modify `webserv-42-paalexan-jopedro-defense-ready-roadmap.20260601-1551.roadforge.json` only after tests pass, marking `WS-412` done; mark `WS-413` done only if the runtime connection is also fully tested.

---

### Task 1: HttpResponse Serialization

**Files:**
- Create: `include/http/HttpResponse.hpp`
- Create: `src/http/HttpResponse.cpp`
- Create: `tests/http/test_http_response.cpp`
- Modify: `Makefile`

- [ ] **Step 1: Write the failing response serialization tests**

Add `tests/http/test_http_response.cpp` with tests in the existing plain-C++ style:

```cpp
#include "http/HttpResponse.hpp"
#include "http/ResponseBuilder.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

static void assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static void test_serializes_status_headers_and_body(void)
{
	HttpResponse response;

	response.setStatus(201, "Created");
	response.setHeader("Content-Type", "text/plain");
	response.setBody("hello");
	response.setCloseConnection(true);

	assert_true(response.serialize()
		== "HTTP/1.1 201 Created\r\n"
		"Connection: close\r\n"
		"Content-Length: 5\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"hello",
		"response should serialize status line, headers, and body");
}

static void test_content_length_tracks_body_size(void)
{
	HttpResponse response;

	response.setStatus(200, "OK");
	response.setBody("abcdef");

	assert_true(response.serialize().find("Content-Length: 6\r\n")
		!= std::string::npos,
		"response should derive Content-Length from body size");
}

static void test_default_reason_phrase(void)
{
	HttpResponse response;

	response.setStatus(404);
	response.setBody("missing");

	assert_true(response.serialize().find("HTTP/1.1 404 Not Found\r\n") == 0,
		"response should supply default reason phrase");
}

int main(void)
{
	try
	{
		test_serializes_status_headers_and_body();
		test_content_length_tracks_body_size();
		test_default_reason_phrase();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_http_response: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_http_response: OK" << std::endl;
	return (0);
}
```

- [ ] **Step 2: Wire the failing test target**

Modify `Makefile` so it knows:

```make
TEST_HTTP_RESPONSE	:= tests/http/test_http_response

HTTP_RESPONSE_SRCS	:= \
	$(SRC_DIR)/http/HttpResponse.cpp \
	$(SRC_DIR)/http/ResponseBuilder.cpp
```

Add `$(SRC_DIR)/http/HttpResponse.cpp` and `$(SRC_DIR)/http/ResponseBuilder.cpp` to `COMMON_SRCS`.

Add a target:

```make
$(TEST_HTTP_RESPONSE): tests/http/test_http_response.cpp $(HTTP_SRCS) $(HTTP_RESPONSE_SRCS)
	$(CXX) $(CXXFLAGS) $(INC) tests/http/test_http_response.cpp $(HTTP_SRCS) $(HTTP_RESPONSE_SRCS) -o $(TEST_HTTP_RESPONSE)
```

Update `test_http_internal`:

```make
test_http_internal: $(TEST_HTTP_REQUEST) $(TEST_HTTP_RESPONSE)
	./$(TEST_HTTP_REQUEST)
	./$(TEST_HTTP_RESPONSE)
```

Update `fclean` to remove `$(TEST_HTTP_RESPONSE)`.

- [ ] **Step 3: Run the new test and verify it fails**

Run:

```sh
make test_http_internal
```

Expected: compilation fails because `http/HttpResponse.hpp` and `http/ResponseBuilder.hpp` do not exist yet.

- [ ] **Step 4: Implement `HttpResponse` minimally**

Create `include/http/HttpResponse.hpp`:

```cpp
#ifndef HTTP_RESPONSE_HPP
# define HTTP_RESPONSE_HPP

# include <map>
# include <string>

class HttpResponse
{
public:
	typedef std::map<std::string, std::string> HeaderMap;

	HttpResponse(void);

	void				setStatus(unsigned int code);
	void				setStatus(unsigned int code,
							const std::string &reason);
	void				setHeader(const std::string &name,
							const std::string &value);
	void				setBody(const std::string &body);
	void				setCloseConnection(bool closeConnection);

	unsigned int		statusCode(void) const;
	const std::string	&reasonPhrase(void) const;
	const std::string	&body(void) const;
	bool				closeConnection(void) const;
	std::string			serialize(void) const;

private:
	unsigned int	_statusCode;
	std::string		_reasonPhrase;
	HeaderMap		_headers;
	std::string		_body;
	bool			_closeConnection;

	static std::string	defaultReasonPhrase(unsigned int code);
	static std::string	sizeToString(std::size_t value);
};

#endif
```

Create `src/http/HttpResponse.cpp`:

```cpp
#include "http/HttpResponse.hpp"

#include <sstream>

HttpResponse::HttpResponse(void) :
	_statusCode(200),
	_reasonPhrase("OK"),
	_headers(),
	_body(),
	_closeConnection(false)
{
}

void HttpResponse::setStatus(unsigned int code)
{
	_statusCode = code;
	_reasonPhrase = defaultReasonPhrase(code);
}

void HttpResponse::setStatus(unsigned int code, const std::string &reason)
{
	_statusCode = code;
	_reasonPhrase = reason;
}

void HttpResponse::setHeader(const std::string &name,
	const std::string &value)
{
	_headers[name] = value;
}

void HttpResponse::setBody(const std::string &body)
{
	_body = body;
}

void HttpResponse::setCloseConnection(bool closeConnection)
{
	_closeConnection = closeConnection;
}

unsigned int HttpResponse::statusCode(void) const
{
	return (_statusCode);
}

const std::string &HttpResponse::reasonPhrase(void) const
{
	return (_reasonPhrase);
}

const std::string &HttpResponse::body(void) const
{
	return (_body);
}

bool HttpResponse::closeConnection(void) const
{
	return (_closeConnection);
}

std::string HttpResponse::serialize(void) const
{
	std::ostringstream output;
	HeaderMap headers(_headers);

	headers["Content-Length"] = sizeToString(_body.size());
	if (_closeConnection)
		headers["Connection"] = "close";
	output << "HTTP/1.1 " << _statusCode << " " << _reasonPhrase << "\r\n";
	for (HeaderMap::const_iterator it = headers.begin(); it != headers.end();
		++it)
		output << it->first << ": " << it->second << "\r\n";
	output << "\r\n";
	output << _body;
	return (output.str());
}

std::string HttpResponse::defaultReasonPhrase(unsigned int code)
{
	if (code == 200)
		return ("OK");
	if (code == 400)
		return ("Bad Request");
	if (code == 404)
		return ("Not Found");
	if (code == 405)
		return ("Method Not Allowed");
	if (code == 431)
		return ("Request Header Fields Too Large");
	if (code == 500)
		return ("Internal Server Error");
	if (code == 505)
		return ("HTTP Version Not Supported");
	return ("");
}

std::string HttpResponse::sizeToString(std::size_t value)
{
	std::ostringstream output;

	output << value;
	return (output.str());
}
```

- [ ] **Step 5: Run response serialization tests**

Run:

```sh
make test_http_internal
```

Expected: `test_http_request_parser: OK` and `test_http_response: OK`.

---

### Task 2: ResponseBuilder Minimal Response

**Files:**
- Create: `include/http/ResponseBuilder.hpp`
- Create: `src/http/ResponseBuilder.cpp`
- Modify: `tests/http/test_http_response.cpp`

- [ ] **Step 1: Add failing builder tests**

Extend `tests/http/test_http_response.cpp`:

```cpp
static void test_builder_creates_simple_close_response(void)
{
	HttpRequest request;
	ResponseBuilder builder;
	HttpResponse response;
	std::string serialized;

	request.setMethod(HTTP_METHOD_GET);
	request.setTarget("/hello");
	request.setVersion("HTTP/1.1");
	response = builder.buildSimpleResponse(request);
	serialized = response.serialize();

	assert_true(serialized.find("HTTP/1.1 200 OK\r\n") == 0,
		"builder should create 200 OK");
	assert_true(serialized.find("Content-Type: text/plain\r\n")
		!= std::string::npos,
		"builder should set text/plain");
	assert_true(serialized.find("Connection: close\r\n") != std::string::npos,
		"builder should close connections for now");
	assert_true(serialized.find("webserv response\n") != std::string::npos,
		"builder should include minimal body");
}
```

Call `test_builder_creates_simple_close_response()` from `main()`.

- [ ] **Step 2: Run the builder test and verify it fails**

Run:

```sh
make test_http_internal
```

Expected: compilation fails because `ResponseBuilder` is not implemented.

- [ ] **Step 3: Implement `ResponseBuilder` minimally**

Create `include/http/ResponseBuilder.hpp`:

```cpp
#ifndef RESPONSE_BUILDER_HPP
# define RESPONSE_BUILDER_HPP

# include "http/HttpRequest.hpp"
# include "http/HttpResponse.hpp"

class ResponseBuilder
{
public:
	ResponseBuilder(void);

	HttpResponse	buildSimpleResponse(const HttpRequest &request) const;
	HttpResponse	buildErrorResponse(unsigned int statusCode) const;

private:
	static std::string	defaultBody(unsigned int statusCode,
							const std::string &target);
};

#endif
```

Create `src/http/ResponseBuilder.cpp`:

```cpp
#include "http/ResponseBuilder.hpp"

#include <sstream>

ResponseBuilder::ResponseBuilder(void)
{
}

HttpResponse ResponseBuilder::buildSimpleResponse(
	const HttpRequest &request) const
{
	HttpResponse response;

	response.setStatus(200);
	response.setHeader("Content-Type", "text/plain");
	response.setCloseConnection(true);
	response.setBody(defaultBody(200, request.getTarget()));
	return (response);
}

HttpResponse ResponseBuilder::buildErrorResponse(unsigned int statusCode) const
{
	HttpResponse response;

	response.setStatus(statusCode);
	response.setHeader("Content-Type", "text/plain");
	response.setCloseConnection(true);
	response.setBody(defaultBody(statusCode, ""));
	return (response);
}

std::string ResponseBuilder::defaultBody(unsigned int statusCode,
	const std::string &target)
{
	std::ostringstream body;

	body << "webserv response\n";
	body << "status: " << statusCode << "\n";
	if (!target.empty())
		body << "target: " << target << "\n";
	return (body.str());
}
```

- [ ] **Step 4: Run builder tests**

Run:

```sh
make test_http_internal
```

Expected: request parser tests and response tests pass.

---

### Task 3: Queue Responses For PROCESSING Clients

**Files:**
- Modify: `include/runtime/RuntimeLoop.hpp`
- Modify: `src/runtime/RuntimeLoop.cpp`
- Modify: `tests/runtime/test_runtime_loop.cpp`
- Modify: `Makefile`

- [ ] **Step 1: Add failing runtime integration test**

Extend `tests/runtime/test_runtime_loop.cpp` with:

```cpp
static void test_processing_client_gets_response_and_closes(void)
{
	int					fds[2];
	EventLoop			eventLoop;
	ClientManager		clients;
	ClientIo			io(1024);
	RuntimeLoop			runtime(eventLoop, clients, io);
	std::string			response;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(fds[0]);
	set_non_blocking(fds[1]);
	clients.connections().push_back(ClientConnection(fds[0], 10));
	eventLoop.registerClient(clients.connections().back());
	write(fds[1], "GET /hello HTTP/1.1\r\nHost: localhost\r\n\r\n", 40);

	runtime.runCycles(3, 100);
	response = read_available(fds[1]);

	close_fd(fds[1]);
	assert_true(response.find("HTTP/1.1 200 OK\r\n") == 0,
		"runtime should write a 200 response");
	assert_true(response.find("Connection: close\r\n") != std::string::npos,
		"runtime response should ask to close");
	assert_true(clients.connections().empty(),
		"runtime should close client after response drains");
}
```

Call `test_processing_client_gets_response_and_closes()` from `main()`.

- [ ] **Step 2: Update runtime-loop test link sources**

Modify `$(TEST_RUNTIME_LOOP)` in `Makefile` to include `$(HTTP_RESPONSE_SRCS)` because `RuntimeLoop.cpp` will use `ResponseBuilder`:

```make
$(TEST_RUNTIME_LOOP): tests/runtime/test_runtime_loop.cpp $(CLIENT_CONNECTION_SRCS) $(HTTP_RESPONSE_SRCS) $(SRC_DIR)/runtime/ClientIo.cpp $(SRC_DIR)/runtime/ClientManager.cpp $(SRC_DIR)/runtime/EventLoop.cpp $(SRC_DIR)/runtime/RuntimeLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_runtime_loop.cpp $(CLIENT_CONNECTION_SRCS) $(HTTP_RESPONSE_SRCS) $(SRC_DIR)/runtime/ClientIo.cpp $(SRC_DIR)/runtime/ClientManager.cpp $(SRC_DIR)/runtime/EventLoop.cpp $(SRC_DIR)/runtime/RuntimeLoop.cpp -o $(TEST_RUNTIME_LOOP)
```

- [ ] **Step 3: Run the runtime test and verify it fails**

Run:

```sh
make test_runtime_internal
```

Expected: `test_runtime_loop` fails because no response is queued for `PROCESSING` clients.

- [ ] **Step 4: Add runtime response helpers**

Modify `include/runtime/RuntimeLoop.hpp`:

```cpp
# include "http/ResponseBuilder.hpp"
```

Add private helpers:

```cpp
void	queueProcessingResponses(void);
void	closeIfResponseComplete(ClientConnection &client);
```

Modify `src/runtime/RuntimeLoop.cpp`:

```cpp
#include "http/HttpResponse.hpp"
#include "http/ResponseBuilder.hpp"
```

Call `queueProcessingResponses()` in `runCycle()` after all ready events have been handled and before `removeClosing()`:

```cpp
for (std::size_t i = 0; i < events.size(); ++i)
	handleEvent(events[i], stats);
queueProcessingResponses();
stats.removedClients += _clients.removeClosing(_eventLoop);
```

In `handleClientEvent()`, close drained responses after writable I/O:

```cpp
result = _io.handleWritable(event);
stats.bytesWritten += result.bytes;
if (event.client != NULL)
	closeIfResponseComplete(*event.client);
```

Implement:

```cpp
void RuntimeLoop::queueProcessingResponses(void)
{
	ResponseBuilder builder;

	for (ClientManager::ConnectionList::iterator it =
		_clients.connections().begin(); it != _clients.connections().end();
		++it)
	{
		if (it->state() == ClientConnection::PROCESSING
			&& it->hasParsedRequest())
		{
			HttpResponse response = builder.buildSimpleResponse(
				it->getParsedRequest());
			it->appendWriteData(response.serialize());
			it->setState(ClientConnection::WRITING_RESPONSE);
		}
	}
}

void RuntimeLoop::closeIfResponseComplete(ClientConnection &client)
{
	if (client.state() == ClientConnection::WRITING_RESPONSE
		&& client.writeBuffer().empty())
		client.closeWithReason("response complete");
}
```

- [ ] **Step 5: Run runtime tests**

Run:

```sh
make test_runtime_internal
```

Expected: all runtime tests pass, including the new request-to-response path.

---

### Task 4: Startup Smoke And Curl Check

**Files:**
- Modify: `tests/run.py` only if an existing active test needs a command update.
- No new static-file/routing fixtures.

- [ ] **Step 1: Run full active suite**

Run:

```sh
python3 tests/run.py all --no-color
```

Expected: build, config, runtime, and HTTP tests pass; planned HTTP/CGI/stress sections remain skipped if still marked planned.

- [ ] **Step 2: Manual runtime check**

Run server:

```sh
./webserv configs/default.conf
```

In another shell:

```sh
curl -i http://127.0.0.1:8080/
```

Expected response shape:

```text
HTTP/1.1 200 OK
Connection: close
Content-Length: <number>
Content-Type: text/plain

webserv response
status: 200
target: /
```

Stop the server with Ctrl+C.

---

### Task 5: Documentation And Roadmap

**Files:**
- Modify: `tests/http/README.md`
- Modify: `docs/runtime_foundation.md`
- Modify: `webserv-42-paalexan-jopedro-defense-ready-roadmap.20260601-1551.roadforge.json`

- [ ] **Step 1: Document response foundation scope**

Update `tests/http/README.md` to mention:

```markdown
- `tests/http/test_http_response.cpp` covers response status/header/body serialization and the minimal `ResponseBuilder`.
```

Update `docs/runtime_foundation.md` to state:

```markdown
`RuntimeLoop` now queues a minimal close-after-response HTTP response when a parsed request reaches `PROCESSING`. This is a response foundation only; routing, static files, custom error pages, CGI, uploads, DELETE, and body parsing remain later tasks.
```

- [ ] **Step 2: Mark roadmap truthfully**

After all validation passes:

- Mark `WS-412.done` as `true`.
- Mark `WS-412.next` as `false`.
- Mark `WS-413.done` as `true` only if the runtime integration test and manual curl check both pass.
- Do not mark `WS-405` done.
- Leave Alexandre-owned `WS-410` unchanged.

---

### Task 6: Final Verification And Commit

**Files:**
- No code changes unless a verification failure requires a fix.

- [ ] **Step 1: Run required verification**

Run:

```sh
make fclean
make
make
make lint
python3 tests/run.py all --no-color
```

- [ ] **Step 2: Manual server verification**

Run `./webserv configs/default.conf`, confirm it stays alive, run `curl -i http://127.0.0.1:8080/`, then stop with Ctrl+C.

- [ ] **Step 3: Clean and inspect**

Run:

```sh
make fclean
git status --short
make diff
```

- [ ] **Step 4: Commit**

If validation passes and the roadmap was updated truthfully:

```sh
git add include/http/HttpResponse.hpp src/http/HttpResponse.cpp include/http/ResponseBuilder.hpp src/http/ResponseBuilder.cpp tests/http/test_http_response.cpp include/runtime/RuntimeLoop.hpp src/runtime/RuntimeLoop.cpp tests/runtime/test_runtime_loop.cpp Makefile tests/http/README.md docs/runtime_foundation.md webserv-42-paalexan-jopedro-defense-ready-roadmap.20260601-1551.roadforge.json
git commit -m "feat: add response builder runtime path"
```

---

## Self-Review

- Spec coverage: WS-412 is covered by `HttpResponse`, `ResponseBuilder`, serialization tests, and docs. WS-413 is covered by runtime queuing, write/close behavior, runtime tests, and curl verification.
- Out of scope preserved: no body parsing, routing, static files, custom error pages, uploads, DELETE, CGI, or chunked decoding.
- Type consistency: `HttpResponse`, `ResponseBuilder`, `RuntimeLoop`, `ClientConnection::PROCESSING`, and `ClientConnection::WRITING_RESPONSE` names match current `main`.
- Risk note: `RuntimeLoop` must queue responses outside poll readiness because `PROCESSING` clients have no read or write interest until response bytes are appended.
