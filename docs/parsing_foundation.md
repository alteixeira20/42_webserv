# Parsing Foundation

This document describes the parsing foundation implemented on `feature/parsing-foundation`. It documents the types and boundaries that exist now, plus the intended handoff to the next parser and runtime branches.

It does not claim that full semantic config parsing is implemented. On this branch, `ConfigParser` wires the startup pipeline from file/string input through tokenization into parser state and currently returns an empty `Config`.

## Scope And Ownership

Status: Implemented

- Alexandre / `paalexan`: config parsing foundation, HTTP method representation, future HTTP parsing and route resolution.
- Farreca / `jopedro-`: runtime, sockets, event loop, non-blocking I/O.
- Shared: CGI integration and final wiring between parsed config and runtime.

Status: Not responsible for

- This branch does not implement socket setup, the event loop, HTTP request parsing, response generation, filesystem serving, uploads, DELETE behavior, or CGI execution.
- This branch does not implement complete parsing of `server` and `location` directives.

## Implemented Pipeline

Status: Implemented

The current parsing pipeline is:

```text
config path -> file content -> ConfigTokenizer -> vector<ConfigToken>
            -> ConfigParser parser state -> Config
```

`ConfigParser::parseFile()` opens and reads a file, then delegates to `parseString()`. `parseString()` tokenizes the content and stores the token stream in parser state through `reset()`. The semantic parser will be built on top of this state in the next branch.

Known limitation: at this stage, `parseFile()` proves the file-read and tokenization pipeline only. It does not yet validate that the file contains a usable server block.

Important invariants:

- Invalid config is a startup concern, not a per-request HTTP behavior.
- Parser state owns its token vector; later parser code must not keep references to temporary tokenizer output.
- Runtime side effects must stay outside config parsing. Parsing must not open sockets, execute CGI, or touch request handling state.

Status: Planned

- Parse block structure and directives into `ServerConfig`, `RouteConfig`, and `ListenConfig`.
- Validate syntax and semantics before runtime starts binding sockets.
- Throw `ConfigException` with useful line/column information when parsing or validation fails.

## HttpMethod

Status: Implemented

Purpose: shared representation for the mandatory methods `GET`, `POST`, and `DELETE`, plus `HTTP_METHOD_UNKNOWN`.

Why it exists: config parsing, HTTP parsing, and route permission checks need one common method vocabulary. Unknown methods must not crash the server; they must remain representable so later layers can reject or handle them deliberately.

What it does:

- Defines the `HttpMethod` enum.
- Converts strings to methods with `parseHttpMethod()`.
- Converts methods back to text with `httpMethodToStr()`.
- Identifies supported methods with `isSupportedHttpMethod()`.
- Checks method vectors with `containsHttpMethod()`.

What it does not do:

- It does not parse full HTTP requests.
- It does not enforce route permissions.
- It does not generate `405 Method Not Allowed`.
- It does not decide how runtime responds to an unknown method.

Important invariants:

- `HTTP_METHOD_UNKNOWN` is not a supported method.
- Supported method names are uppercase.
- Method lists should not rely on duplicate entries.
- The representation is an enum plus free functions, not a class or template.

How it supports evaluation: evaluators will test `GET`, `POST`, `DELETE`, and possibly invalid methods. This type gives config and HTTP parsing a shared, non-crashing representation for those cases.

How it connects to the next branch: config parsing should reject unknown methods inside `allowed_methods` or equivalent route directives, while HTTP request parsing can still represent an unknown request method safely.

## ConfigToken

Status: Implemented

Purpose: one lexical token produced by the tokenizer.

Why it exists: parser code needs a small typed value with source location instead of raw character scanning. Location data is required for useful startup errors.

What it stores:

- token type;
- token value;
- source line;
- source column.

Recognized token types are `WORD`, `OPEN_BRACE`, `CLOSE_BRACE`, `SEMICOLON`, and `END`.

What it does not do:

- It does not know directive names.
- It does not validate directive scope or argument count.
- It does not normalize paths, ports, sizes, methods, or status codes.

Important invariants:

- From parser code, tokens are read through const getters.
- Line and column identify the token start.
- `CONFIG_TOKEN_END` marks the end of the stream.

How it supports evaluation: malformed configs should fail with a useful location instead of a vague startup error.

How it connects to the next branch: semantic parser errors should use the current token's line and column when throwing `ConfigException`.

## ConfigException

Status: Implemented

Purpose: startup configuration error type with message, line, and column.

Why it exists: invalid config must stop startup cleanly before runtime opens sockets or initializes partial server state.

What it does:

- Extends `std::exception`.
- Stores an error message.
- Stores line and column values.
- Exposes the message through `what()`.

What it does not do:

- It does not recover from invalid config.
- It does not collect multiple errors.
- It does not decide process exit policy.
- It does not format multi-line diagnostics by itself.

Important invariants:

- A `ConfigException` represents a startup config failure.
- `getLine()` and `getColumn()` preserve the location supplied by the throwing parser or tokenizer code.
- Missing-file and read-file errors currently use line `0`, column `0`.

How it supports evaluation: malformed config files can fail deterministically before runtime state is created.

How it connects to the next branch: every syntax or semantic config failure should use this type so the executable can print a consistent startup error.

## ConfigTokenizer

Status: Implemented

Purpose: converts config file text into `ConfigToken` objects.

Why it exists: lexical scanning is separate from grammar and directive semantics, which keeps both tokenizer tests and parser errors easier to reason about.

What it does:

- Tracks content, current index, line, and column.
- Ignores whitespace.
- Ignores `#` comments until newline.
- Recognizes `{`, `}`, and `;` as standalone tokens.
- Reads all other contiguous non-whitespace, non-comment, non-punctuation text as `WORD`.
- Appends a final `END` token.

What it does not do:

- It does not recognize `server`, `listen`, `root`, or any directive name.
- It does not parse strings, ports, paths, body sizes, methods, or status codes.
- It does not validate block structure.
- It does not decide whether a word is legal in the current parser state.

Important invariants:

- `tokenize()` resets tokenizer state for the provided content.
- Comments do not produce tokens.
- Punctuation tokens use their source position.
- Parser owns all semantic meaning.

How it supports evaluation: the config layer can reject malformed files at startup with deterministic token positions, instead of relying on fragile string splitting.

How it connects to the next branch: parser tests should cover comments, whitespace, braces, semicolons, words, and EOF before semantic directive tests are added.

## ListenConfig

Status: Implemented

Purpose: value object for one listen endpoint: host plus port.

Why it exists: runtime needs typed endpoint data before it can bind sockets, and the subject requires multiple ports and host/interface configuration.

What it does:

- Stores host string and port number.
- Exposes getters and setters.
- Compares endpoints with exact `host` and `port` equality through `equals()`.

What it does not do:

- It does not open sockets.
- It does not validate port range.
- It does not resolve hostnames.
- It does not detect wildcard overlap such as `0.0.0.0:8080` versus `127.0.0.1:8080`.

Important invariants:

- One instance means one configured host:port pair.
- Equality is exact string equality for host and exact integer equality for port.
- Default construction uses host `0.0.0.0` and port `0` until parser validation supplies accepted values.

How it supports evaluation: maps directly to the requirement to configure all interface:port pairs.

How it connects to the next branch: `listen` parsing must validate host and port before runtime receives the object.

## RouteConfig

Status: Implemented

Purpose: stores route-level configuration.

Why it exists: the subject requires route-specific behavior for allowed methods, roots, directory index, autoindex, redirects, uploads, and CGI.

What it stores:

- route path;
- allowed methods;
- optional route root;
- optional index;
- optional autoindex flag;
- optional redirect status and target;
- optional upload directory;
- CGI extension-to-executable map.

What it does not do:

- It does not match request URLs.
- It does not apply server-level inheritance.
- It does not check filesystem access.
- It does not execute CGI.
- It does not store uploads.
- It does not generate HTTP responses.

Important invariants:

- `hasRoot()`, `hasIndex()`, `hasAutoIndex()`, `hasRedirect()`, and `hasUploadDir()` distinguish omitted directives from explicitly configured falsey values.
- `addAllowedMethod()` avoids duplicate method entries.
- Adding a CGI handler for an existing extension replaces that extension's executable path.
- A route object may be incomplete until parser validation and inheritance are implemented.

How it supports evaluation: it holds the data required to demonstrate every mandatory route-level rule: methods, root override, index, autoindex, redirect, upload directory, and CGI mapping.

How it connects to the next branch: semantic parsing must fill this object from `location` blocks, reject invalid values, and later route resolution must use the typed fields instead of reparsing directive strings.

## ServerConfig

Status: Implemented

Purpose: value object for one server block.

Why it exists: the project needs multiple server definitions, each with its own listen endpoints, names, defaults, error pages, body-size limit, and routes.

What it stores:

- multiple `ListenConfig` endpoints;
- server names;
- server root;
- server index;
- client max body size;
- error pages by status code;
- route configs.

What it does not do:

- It does not bind sockets.
- It does not select the server for a request.
- It does not validate directive syntax.
- It does not apply route inheritance.
- It does not serve default files or error pages.

Important invariants:

- One instance represents one server block.
- A server may contain multiple listen endpoints.
- `addListen()` ignores exact duplicate endpoints inside the same server object.
- `addServerName()` ignores duplicate names inside the same server object.
- The default index is `index.html`.
- The default client max body size is `1000000`.

How it supports evaluation: stores the server-level data required for multiple ports, different hostnames, error pages, body-size limits, static roots, and route collections.

How it connects to the next branch: server-block parsing must populate and validate this object before runtime consumes it.

## Config

Status: Implemented

Purpose: top-level owner for all parsed server blocks.

Why it exists: startup needs one object that can be handed from config parsing to runtime setup.

What it does:

- Owns all `ServerConfig` objects.
- Exposes all servers through `getServers()`.
- Computes unique listen endpoints through `getUniqueListens()`.

What it does not do:

- It does not parse files.
- It does not validate server blocks.
- It does not bind sockets.
- It does not select virtual hosts.
- It does not apply route inheritance.

Important invariants:

- Multiple server blocks may share the same exact host:port endpoint.
- `getUniqueListens()` deduplicates endpoints across all servers using `ListenConfig::equals()`.
- Runtime should bind one socket per unique endpoint, not one socket per server block.

How it supports evaluation: allows multiple configured websites or server blocks to share a port for `server_name` routing without causing duplicate bind attempts.

How it connects to the next branch: once semantic parsing is implemented, `Config` becomes the object passed to runtime initialization and later server selection logic.

## Handoff To Runtime Track

Status: Implemented

`Config::getUniqueListens()` exists specifically for runtime startup.

The intended runtime flow is:

```text
Config
  -> getUniqueListens()
  -> bind one non-blocking listening socket per unique host:port
  -> keep mapping from endpoint to all ServerConfig objects using that endpoint
```

This matters because several server blocks may intentionally share one endpoint:

```text
server A: listen 127.0.0.1:8080; server_name a.local;
server B: listen 127.0.0.1:8080; server_name b.local;
```

Runtime must bind `127.0.0.1:8080` once. After accepting a connection on that socket, later routing can use the request `Host` header and configured `server_name` values to choose the effective `ServerConfig`.

Status: Planned

- Keep an endpoint-to-server-list mapping after binding.
- Decide default-server behavior for an endpoint when `Host` is missing or does not match any configured `server_name`.
- Validate duplicate or ambiguous host:port plus `server_name` combinations during config parsing before runtime setup.

Status: Not responsible for

- `getUniqueListens()` does not create sockets.
- It does not decide wildcard binding policy.
- It does not perform `server_name` matching.
- It does not guarantee that the OS will allow a bind.

Evaluation-critical runtime constraints:

- The final runtime must use only one `poll`, `select`, or equivalent call in the main loop.
- That call must monitor read and write readiness at the same time.
- Sockets and CGI pipes must not be read from or written to unless readiness was reported.
- `errno` must not be used after `read`, `recv`, `write`, or `send` to decide normal I/O behavior.

## Next Branch: feature/config-parser

Status: Planned

The next branch should turn the foundation into a semantic parser. Remaining work:

- parse server blocks;
- parse listen directives;
- parse `server_name`;
- parse `root`, `index`, `client_max_body_size`, and `error_page`;
- parse location blocks;
- parse `allowed_methods` or the final chosen directive name;
- parse `autoindex`;
- parse redirects;
- parse upload directory directives;
- parse CGI extension-to-executable mappings;
- validate duplicate or ambiguous host:port plus `server_name` configs;
- validate missing roots, listens, and routes according to final grammar rules;
- implement route/server lookup later, outside the raw data holders.

Validation should reject misspelled directives, wrong directive scope, invalid argument counts, malformed numbers, unsupported methods, invalid redirect statuses, invalid CGI extension syntax, and duplicate route paths where the final grammar forbids them.

## Cross-Module Summary

Status: Implemented

- `ConfigTokenizer` produces `ConfigToken` values.
- `ConfigParser` stores the token stream and currently returns an empty `Config`.
- `RouteConfig` uses `HttpMethod` for allowed methods.
- `ServerConfig` owns `ListenConfig` and `RouteConfig` values.
- `Config` owns `ServerConfig` values and computes unique `ListenConfig` endpoints.

Status: Not responsible for

- These modules are not the runtime.
- These modules are not the route resolver.
- These modules are not CGI execution.
- These modules are not HTTP response generation.
