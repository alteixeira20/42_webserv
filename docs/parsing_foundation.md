# Parsing Foundation Modules

This document describes the implemented parsing foundation on this branch. It does not describe full config parsing or runtime behavior unless the module already exposes the required data for it.

## HttpMethod

Status: Implemented

Purpose: represents the HTTP methods currently recognized by the project: `GET`, `POST`, and `DELETE`, plus `HTTP_METHOD_UNKNOWN` for invalid or unsupported input.

Why it exists: the subject requires support for `GET`, `POST`, and `DELETE`. Config parsing, HTTP parsing, and route validation need one shared representation of those methods.

What it does: provides an enum and free functions to parse a string into a method, convert a method back to text, check whether a method is supported, and check whether a vector already contains a method.

Not responsible for: parsing HTTP requests, enforcing route permissions, generating `405 Method Not Allowed`, or deciding whether an unknown method is a syntax error or an unsupported-method error.

Important invariants:
- `HTTP_METHOD_UNKNOWN` is not a supported method.
- Supported methods are uppercase only.
- Method lists should not rely on duplicate entries.

Subject/evaluation support: gives both the HTTP layer and config layer a common vocabulary for the mandatory methods evaluators will test.

Design decisions:
- Uses an enum plus free functions, not a class or template.
- Keeps domain-specific logic explicit and simple for C++98.
- Templates are reserved for genuinely generic utilities, not forced into domain-specific code.

Status: Planned

- Parser-level validation should reject methods that parse to `HTTP_METHOD_UNKNOWN` inside route config.
- Runtime should use the same enum to compare request methods against route methods.

## ConfigToken

Status: Implemented

Purpose: stores one tokenizer result: token type, string value, source line, and source column.

Why it exists: parser errors need stable source locations, and the parser needs a small typed object instead of raw characters.

What it does: models word tokens, open brace, close brace, semicolon, and end-of-file. Exposes read-only getters for parser use.

Not responsible for: knowing directive names, validating directive arguments, normalizing paths, or deciding whether a token is legal in a specific parser state.

Important invariants:
- From the parser's point of view, `ConfigToken` is immutable: parser code reads tokens through const getters.
- Line and column describe where the token starts.
- `CONFIG_TOKEN_END` marks the end of the token stream.

Subject/evaluation support: enables precise config diagnostics instead of a generic startup failure, which matters when evaluators provide malformed config files.

Design decisions:
- Token values are stored as strings even for punctuation so debugging and error messages stay simple.
- Token semantics are intentionally shallow; the parser owns config meaning.

## ConfigException

Status: Implemented

Purpose: represents startup configuration errors with a message and source location.

Why it exists: invalid config must fail startup cleanly before sockets are opened or runtime state is partially initialized.

What it does: extends `std::exception`, stores an error message, and stores line and column values accessible through getters.

Not responsible for: recovering from invalid config, formatting multi-line diagnostics, collecting multiple errors, or deciding process exit policy.

Important invariants:
- `what()` returns the stored message.
- `getLine()` and `getColumn()` preserve the location supplied by the throwing parser/tokenizer code.
- A config error is a startup error, not a per-request HTTP response.

Subject/evaluation support: supports resilience by making malformed configuration a controlled startup failure rather than undefined behavior.

Design decisions:
- Used for startup config errors with line/column context.
- Keeps exception payload small: message, line, column.
- Formatting policy remains outside the exception object.

Status: Planned

- Parser code should throw `ConfigException` when syntax or semantic validation fails.
- The executable should catch it at startup and print message plus line/column.

## ConfigTokenizer

Status: Implemented

Purpose: converts config file text into a vector of `ConfigToken` objects.

Why it exists: config parsing is easier to test and explain when lexical scanning is separate from grammar and directive validation.

What it does: scans text statefully, tracks index/line/column, skips whitespace and `#` comments, recognizes words, `{`, `}`, `;`, and appends an EOF token.

Not responsible for: validating block structure, recognizing directive names, parsing ports or sizes, validating routes, applying inheritance, or checking filesystem paths.

Important invariants:
- Tokenizer recognizes only words, braces, semicolons, comments, and EOF.
- Parser owns all semantics.
- Each call to `tokenize()` resets tokenizer state to the beginning of the supplied content.
- Comments run from `#` to newline and do not produce tokens.

Subject/evaluation support: provides the first line of defense for config startup behavior and enables useful errors for syntax issues near a token location.

Design decisions:
- Stateful tokenizer keeps helper functions small and clear in C++98.
- It does not use templates because tokenization here is domain-specific, not generic.
- It keeps the accepted lexical grammar narrow so unsupported syntax fails in the parser instead of being guessed by the scanner.

Status: Planned

- Parser tests should assert token streams for comments, braces, semicolons, words, and EOF.
- Parser code should use token line/column when raising `ConfigException`.

## ListenConfig

Status: Implemented

Purpose: models one listen endpoint: host plus port.

Why it exists: the subject requires defining all interface:port pairs on which the server listens. Runtime needs endpoint objects before it can bind sockets.

What it does: stores host and port, exposes getters/setters, and compares two endpoints through `equals()`.

Not responsible for: opening sockets, resolving hostnames, validating port range, detecting conflicts across servers, or deciding whether `0.0.0.0` overlaps another host.

Important invariants:
- One `ListenConfig` represents one host:port pair.
- Equality is exact host string equality plus exact port equality.
- Default construction uses host `0.0.0.0` and port `0` until parser validation supplies real values.

Subject/evaluation support: maps directly to the mandatory multi-port and multi-interface configuration requirement.

Design decisions:
- Kept as a value object so `ServerConfig` and `Config` can store vectors of endpoints.
- Endpoint uniqueness is handled by exact comparison, not by socket/runtime side effects.

Status: Planned

- Parser validation should reject invalid ports and malformed listen directives before runtime sees them.

## RouteConfig

Status: Implemented

Purpose: stores route-level configuration values parsed from a route block.

Why it exists: the subject requires URL/route-specific rules for methods, root, directory listing, index, redirects, uploads, and CGI.

What it does: stores route path, allowed methods, optional root, optional index, optional autoindex flag, optional redirect, optional upload directory, and CGI handlers by extension.

Not responsible for: matching request URLs, resolving inherited server defaults, checking filesystem access, executing CGI, storing uploaded files, deleting files, or generating HTTP responses.

Important invariants:
- `has*()` flags distinguish an omitted directive from a directive set to a falsey value.
- `addAllowedMethod()` avoids duplicate method entries.
- CGI handlers are keyed by extension; adding the same extension replaces the previous executable path.
- A route object may be incomplete until parser validation and inheritance are applied.

Subject/evaluation support: holds the data needed to demonstrate every mandatory route rule during evaluation.

Design decisions:
- Route config is a data holder, not a route resolver.
- Optional route fields use explicit presence flags to keep inheritance decisions unambiguous.
- CGI is represented as extension-to-executable mapping because the subject asks for CGI execution based on file extension.

Status: Planned

- Parser validation should reject invalid redirect statuses, unsupported methods, invalid CGI extensions, duplicate route paths, and non-redirect routes without an effective root.
- Route resolver should apply longest-prefix matching outside `RouteConfig`.

## ServerConfig

Status: Implemented

Purpose: models one `server` block.

Why it exists: the subject suggests an NGINX-like server section and requires multiple websites/listener configurations in one config file.

What it does: stores multiple listen endpoints, server names, root, index, client max body size, error pages, and route configs.

Not responsible for: binding sockets, selecting a server for an incoming request, validating directive syntax, resolving route inheritance, or serving default error pages.

Important invariants:
- One `ServerConfig` represents one server block.
- A server can contain multiple `ListenConfig` endpoints.
- `addListen()` ignores duplicate endpoints within the same server block.
- `addServerName()` ignores duplicate names within the same server block.
- Default index is `index.html` and default client max body size is `1000000` until parser config changes them.

Subject/evaluation support: stores the server-level defaults and route collection required for static serving, body limits, custom error pages, and multi-listen demos.

Design decisions:
- Server-level data is grouped separately from global config so multiple server blocks can coexist.
- Error pages are keyed by status code for direct lookup by response generation later.
- Multiple listen endpoints live on `ServerConfig` because one server block may listen on more than one host:port.

Status: Planned

- Parser validation should ensure each server has at least one valid listen endpoint.
- Runtime/server selection should use listen endpoint plus optional `server_name` rules outside this object.

## Config

Status: Implemented

Purpose: top-level container for parsed server configuration.

Why it exists: runtime startup needs a single object containing all parsed server blocks and the deduplicated listen endpoints it must bind.

What it does: stores server blocks and exposes `getUniqueListens()` to return each exact host:port pair once across all servers.

Not responsible for: parsing files, validating server blocks, binding sockets, selecting virtual hosts, or applying route inheritance.

Important invariants:
- `Config` can contain multiple `ServerConfig` objects.
- `getUniqueListens()` deduplicates endpoints across all server blocks using `ListenConfig::equals()`.
- Runtime should bind one socket per unique host:port pair, not one socket per server block.

Subject/evaluation support: supports multiple configured websites/listeners while preserving the non-blocking runtime requirement that each actual socket is managed once by the event loop.

Design decisions:
- `Config` exposes unique listen endpoints so runtime binds one socket per host:port.
- Multiple server blocks may share an endpoint for `server_name` routing, but runtime must bind only once.
- The top-level object does not decide request routing; it only exposes the data needed by runtime and route-selection layers.

Status: Planned

- Startup code should reject an empty `Config` after parsing.
- Runtime should keep a mapping from bound endpoint to the server blocks that share it.

## Cross-Module Notes

Status: Implemented

- `ConfigTokenizer` produces `ConfigToken` values.
- `RouteConfig` uses `HttpMethod` for allowed methods.
- `ServerConfig` owns `ListenConfig` and `RouteConfig` values.
- `Config` owns `ServerConfig` values and computes unique `ListenConfig` endpoints.

Status: Planned

- `ConfigParser` should connect these foundations into a complete validated `Config`.
- Parser validation should happen before runtime socket setup.
- Runtime and HTTP layers should consume the typed config objects instead of reparsing directive strings.

Status: Not responsible for

- These modules do not implement the event loop, non-blocking I/O, HTTP request parsing, static file serving, upload storage, CGI execution, or response generation.
