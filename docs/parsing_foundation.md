# Parsing Foundation

This document describes the implemented config parsing foundation on
`feature/config-parser`.

## Responsibility Split

`ConfigParser` owns startup input and syntax:

- `parseFile()` reads config text from disk and delegates to `parseString()`.
- `parseString()` tokenizes input, parses block grammar, builds typed config
  objects, and runs semantic validation before returning.
- `src/config/ConfigParser.cpp` contains parser setup and token navigation.
- `src/config/ConfigParserServer.cpp` parses server blocks and server
  directives.
- `src/config/ConfigParserRoute.cpp` parses `location` blocks and route
  directives.
- `src/config/ConfigParserValues.cpp` converts words into ports, sizes,
  status codes, booleans, and HTTP methods.

`ConfigValidator` owns whole-config semantic checks:

- at least one server exists;
- every server has at least one listen endpoint;
- duplicate listen endpoints inside one server are invalid;
- virtual hosts on the same listen endpoint must be unambiguous;
- route paths are absolute and unique inside a server.

`ConfigResolver` owns runtime lookup only:

- server selection by listen endpoint plus Host/server_name;
- default server fallback to the first server on an endpoint;
- longest route-prefix selection with segment-boundary checks.

None of these classes opens sockets, reads requests, writes responses, touches
filesystem content, stores uploads, or executes CGI.

## Implemented Pipeline

```text
config path or string
    -> ConfigTokenizer
    -> ConfigParser
    -> Config, ServerConfig, ListenConfig, RouteConfig
    -> ConfigValidator
    -> runtime-facing typed Config
```

Invalid configuration throws `ConfigException` during startup parsing or
validation. Missing-file and read-file errors use line `0`, column `0`; syntax
errors use tokenizer locations where possible.

## Data Types

`ConfigToken` stores token type, value, line, and column. Token types are
`WORD`, `OPEN_BRACE`, `CLOSE_BRACE`, `SEMICOLON`, and `END`.

`ListenConfig` stores one host and port endpoint. Port-only `listen` directives
become host `0.0.0.0` plus the parsed port.

`ServerConfig` stores listens, server names, server root, server index, client
body size, error pages, and routes.

`RouteConfig` stores route path, allowed methods, route root, route index,
autoindex, redirect, upload directory, and CGI extension mappings. Optional
route fields expose `has*()` flags so omitted values stay distinguishable from
explicit false or empty values.

`HttpMethod` represents `GET`, `POST`, `DELETE`, and `UNKNOWN`. Config parsing
accepts only the supported methods inside `allowed_methods`.

## Supported Directives

Server scope supports `listen`, `server_name`, `root`, `index`,
`client_max_body_size`, and `error_page`.

Route scope supports `root`, `index`, `autoindex`, `allowed_methods`,
`redirect`, `upload_dir`, and `cgi` inside `location` blocks.

See `docs/config_grammar.md` for exact argument rules and examples.

## Current Limitations

- Body-size suffixes are not implemented; use integer byte counts.
- Values are unquoted tokens.
- Filesystem paths and CGI executables are not checked by the parser.
- Runtime HTTP behavior, route inheritance, filesystem serving, uploads, and CGI
  execution are separate later layers.
