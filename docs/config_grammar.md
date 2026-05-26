# Config Grammar

This document defines the intended configuration language for `webserv`.
It is a parser contract, not an implementation note: if the parser accepts or
rejects something differently, update either the parser or this document before
evaluation.

Current status: on `feature/parsing-foundation`, the tokenizer, config data
holders, and parser pipeline are implemented, but full semantic parsing of
`server` and route directives is planned for the next branch.

## Module Scope

What it does:
the config module reads one configuration file, tokenizes it, parses `server`
and `route` blocks, validates every directive, and exposes normalized server
configuration to the runtime, HTTP layer, route resolver, upload handling, and
CGI handling.

Why it exists:
the executable must be configurable without recompilation. The subject requires
the server to accept a configuration file as an argument or use a default path.

Subject requirements supported:
- define all interface:port pairs to listen on;
- serve multiple websites or listener configurations;
- set default error pages;
- set maximum client request body size;
- configure route rules without regex;
- configure accepted methods, redirects, roots, directory listing, index files,
  upload storage, and CGI execution by extension.

Important invariants:
- parsing either succeeds completely or the server does not start;
- accepted configuration is deterministic and independent of directive order
  except where this document explicitly says otherwise;
- route paths are plain URL prefixes, not regular expressions;
- runtime code receives validated values, not raw parser tokens;
- one invalid directive must not leave a partially usable config object.

Common failure modes:
- accepting misspelled directives silently;
- allowing duplicate or contradictory route rules;
- treating malformed sizes or ports as valid integers;
- forgetting to reject a route without enough information to map a URL to a
  filesystem path, redirect, upload target, or CGI handler;
- implementing behavior that is not documented here.

How to test it:
- parse at least one full valid config covering every directive;
- keep small invalid configs for each directive type;
- compare route behavior against expected path-resolution examples;
- start the server with the default config path and with an explicit path;
- confirm invalid configs fail before sockets are opened.

## File Shape

What it does:
the file contains one or more `server` blocks. Each `server` block owns listener
directives, server-level defaults, and zero or more `route` blocks.

Why it exists:
the subject requires multiple interface:port pairs and route-specific behavior.
Keeping all route rules inside a server block makes listener ownership explicit.

Subject requirement supported:
configuration file inspired by NGINX `server` sections.

Grammar:

```nginx
config        = server_block+
server_block  = "server" "{" server_directive* route_block* "}"
route_block   = "route" path "{" route_directive* "}"
directive     = name argument* ";"
comment       = "#" text_until_end_of_line
```

Important invariants:
- the file must contain at least one `server` block;
- braces must be balanced;
- every directive ends with `;`;
- comments are ignored outside quoted strings;
- unknown blocks are rejected.

Common failure modes:
- missing `;` after a directive;
- missing closing `}`;
- content outside a `server` block;
- nested `server` or nested `route` blocks;
- accepting an empty file.

How to test it:
- valid: one server with one route;
- valid: multiple server blocks;
- invalid: route outside server;
- invalid: extra token after final `}`;
- invalid: missing semicolon, missing opening brace, missing closing brace.

## Lexical Rules

What it does:
the tokenizer turns the file into names, values, `{`, `}`, and `;` tokens while
preserving line numbers for error messages.

Why it exists:
separating tokenization from parsing keeps syntax errors clear and prevents the
parser from depending on fragile string splitting.

Subject requirement supported:
resilience. A malformed config must fail cleanly instead of causing undefined
runtime behavior.

Rules:
- whitespace separates tokens;
- `#` starts a comment until the end of the line;
- `{`, `}`, and `;` are standalone tokens;
- directive names are case-sensitive lowercase words;
- paths are written as unquoted tokens;
- quoted strings are not required for the mandatory grammar.

Important invariants:
- tokenization must never lose line and column context;
- comments do not create empty directives;
- a token cannot contain `{`, `}`, or `;`;
- parser errors report the directive or block being parsed when possible.

Common failure modes:
- treating `root /tmp/www; # ok` as having extra arguments;
- accepting `root/tmp/www;` as directive `root/tmp/www`;
- accepting unclosed quotes if quotes are later added;
- reporting only "invalid config" without line context.

How to test it:
- comments before, after, and between directives;
- multiple directives on separate lines;
- tabs and repeated spaces;
- syntax error near end of file;
- directive immediately followed by `;` with no argument.

## Supported Directives

What it does:
this section is the complete directive list for the mandatory configuration
language. Anything else is invalid unless the team explicitly extends this file.

Why it exists:
evaluation configs should be predictable. A closed directive set also makes
parser validation testable.

Subject requirement supported:
all mandatory config features listed in subject section IV.3.

Server directives:

| Directive | Arguments | Required | Meaning |
| --- | --- | --- | --- |
| `listen` | `host:port` or `port` | yes, one or more | Interface and port to bind. |
| `server_name` | name... | no | Optional virtual-host label if implemented. |
| `root` | filesystem_path | no | Default document root inherited by routes. |
| `error_page` | status path | no | Custom error page for one status code. |
| `client_max_body_size` | size | no | Maximum accepted request body size. |

Route directives:

| Directive | Arguments | Required | Meaning |
| --- | --- | --- | --- |
| `methods` | method... | no | Accepted HTTP methods for the route. |
| `redirect` | status target | no | Return an HTTP redirect instead of serving a file. |
| `root` | filesystem_path | conditional | Filesystem root for this route. |
| `autoindex` | `on` or `off` | no | Enable or disable directory listing. |
| `index` | filename... | no | Default file candidates for directory requests. |
| `upload_store` | filesystem_path | no | Directory where uploaded files are stored. |
| `cgi` | extension executable_path | no | CGI executable for files matching extension. |

Important invariants:
- unknown directives are errors;
- directive names are only valid in their documented block type;
- repeated directives are rejected unless explicitly documented as repeatable;
- `listen`, `error_page`, `index`, and `cgi` may appear more than once;
- `server_name` is optional because the subject says virtual hosts are out of
  scope;
- `GET`, `POST`, and `DELETE` are the only mandatory methods.

Common failure modes:
- accepting `methods` at server scope;
- accepting `listen` inside a route;
- allowing duplicate `root` in the same block;
- treating `server_name` as required;
- implementing virtual-host behavior without documenting it.

How to test it:
- one config using every directive;
- one invalid config per wrong-scope directive;
- duplicate non-repeatable directive;
- repeated `listen`, `error_page`, `index`, and `cgi`;
- unknown directive with a similar spelling.

## Example Config

What it does:
this example demonstrates the intended syntax and all mandatory route features.

Why it exists:
it gives evaluators and developers a compact reference config to compare against
the parser and runtime behavior.

Subject requirement supported:
default files must demonstrate that every feature works during evaluation.

```nginx
server {
    listen 127.0.0.1:8080;
    listen 8081;

    root www/default;
    client_max_body_size 2M;
    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;

    route / {
        methods GET;
        index index.html;
        autoindex off;
    }

    route /public {
        methods GET;
        root www/public;
        autoindex on;
    }

    route /upload {
        methods GET POST DELETE;
        root www/upload;
        upload_store uploads;
        index upload.html;
    }

    route /old {
        methods GET;
        redirect 301 /;
    }

    route /cgi-bin {
        methods GET POST;
        root www/cgi-bin;
        cgi .py /usr/bin/python3;
    }
}
```

Important invariants:
- `/old` is a redirect route and must not also require a filesystem root;
- `/upload` explicitly allows upload storage;
- `/cgi-bin` declares CGI by file extension;
- `/public` can list directories only because `autoindex on` is set.

Common failure modes:
- resolving `/public/a.txt` against the server root instead of route root;
- allowing `POST /public` when only `GET` is configured;
- serving a directory listing when `autoindex off`;
- running CGI for an undeclared extension.

How to test it:
- request `/`, `/public/`, `/upload/`, `/old`, and `/cgi-bin/script.py`;
- send `POST` to a route that only allows `GET`;
- request a missing file and verify the configured `404` page;
- send a body larger than `2M`;
- compare redirect status and `Location` header with the configured target.

## Inheritance Rules

What it does:
server-level values provide defaults for routes. Route-level directives override
only their own route.

Why it exists:
most routes share the same root, error pages, and body limit. Inheritance keeps
the config short while making route-specific exceptions explicit.

Subject requirement supported:
the subject asks for rules on a URL/route "for a website"; server defaults plus
route overrides map naturally to that model.

Rules:
- `root` inherits from server to route unless the route defines its own `root`;
- `error_page` is server-scoped only and applies to every route in that server;
- `client_max_body_size` is server-scoped only and applies to every route;
- `methods` defaults to `GET` when omitted;
- `autoindex` defaults to `off` when omitted;
- `index` defaults to no configured index when omitted;
- `upload_store` is disabled when omitted;
- `cgi` is disabled when omitted;
- `redirect` does not inherit.

Important invariants:
- inheritance must be resolved before runtime request handling;
- route overrides must not mutate server defaults;
- redirect routes are terminal: redirect handling wins over file, upload,
  autoindex, index, and CGI handling;
- a non-redirect route must have an effective `root`.

Common failure modes:
- route without `root` passing validation when server also lacks `root`;
- inherited `methods` unexpectedly allowing `POST` or `DELETE`;
- `autoindex on` leaking from one route to another;
- applying `upload_store` globally instead of only to the declaring route.

How to test it:
- route inherits server `root`;
- route overrides server `root`;
- route with omitted `methods` accepts `GET` and rejects `POST`;
- route with omitted `autoindex` rejects directory listing;
- redirect route works without a root.

## Route Matching And Resolution

What it does:
route matching selects the best route for a request target, then maps the URL to
the configured route behavior.

Why it exists:
the subject requires per-route rules and explicitly says no regex is required.
Prefix matching is enough for mandatory behavior and is easy to explain during
evaluation.

Subject requirement supported:
route-specific root, methods, redirect, autoindex, index, upload, and CGI.

Rules:
- route paths must start with `/`;
- route paths are normalized without trailing `/`, except `/` itself;
- request targets match by longest path prefix;
- a route prefix matches a path segment boundary only;
- query strings are not part of route matching;
- filesystem lookup removes the route prefix before appending the remainder to
  the effective root.

Example:

```text
route /kapouet { root /tmp/www; }
URL /kapouet/pouic/toto/pouet
filesystem path /tmp/www/pouic/toto/pouet
```

Important invariants:
- longest matching route wins;
- `/app` must not match `/apple`;
- route matching must happen before method validation;
- URL decoding and path traversal policy must be handled before filesystem
  access;
- `..` must never escape the configured root.

Common failure modes:
- first-match behavior instead of longest-match behavior;
- matching `/app` against `/apple`;
- appending the full URL path to route root and producing
  `/tmp/www/kapouet/pouic`;
- allowing encoded traversal such as `%2e%2e/`;
- including query string in filesystem path.

How to test it:
- overlapping routes `/`, `/app`, and `/app/static`;
- request `/apple` when only `/app` exists;
- request `/kapouet/pouic/toto/pouet` using the subject example;
- request `/file.txt?x=1`;
- traversal attempts using `../` and encoded dot segments.

## Validation Rules

What it does:
validation checks that the parsed config is complete, unambiguous, and safe to
hand to runtime code.

Why it exists:
runtime code should not need to guess how to handle malformed directives. Invalid
config must fail startup cleanly.

Subject requirement supported:
resilience, accurate status behavior, uploads, multiple ports, route rules, and
CGI execution.

Rules:
- `listen` port must be an integer from `1` to `65535`;
- `listen host` must be an IPv4 address or a supported hostname token;
- duplicate `listen` pairs in the same config are invalid unless deliberately
  used for virtual-host behavior;
- `client_max_body_size` accepts positive integer bytes or suffixes `K`, `M`,
  and `G`;
- `error_page` status must be an HTTP error status, normally `400` to `599`;
- `methods` accepts only implemented methods: `GET`, `POST`, `DELETE`;
- `redirect` status must be a redirect status, normally `301`, `302`, `303`,
  `307`, or `308`;
- `redirect` target must be an absolute URL path or full URL;
- `autoindex` accepts only `on` or `off`;
- `cgi` extension must start with `.`;
- each route path must be unique inside its server;
- a non-redirect route must have an effective `root`;
- upload routes must define `upload_store` if uploads are expected there.

Important invariants:
- numeric overflow is invalid;
- empty argument lists are invalid;
- extra arguments are invalid unless the directive accepts a list;
- validation must not access the network;
- filesystem existence checks should be a deliberate team decision because they
  can make configs environment-dependent.

Common failure modes:
- accepting `client_max_body_size 0;`;
- wrapping large sizes through integer overflow;
- accepting `GET POST FOO`;
- accepting `redirect 200 /x;`;
- accepting `cgi py /usr/bin/python3;` without a leading dot.

How to test it:
- invalid port `0`, `65536`, and non-numeric port;
- invalid body sizes `0`, `-1`, `10Z`, and huge overflow values;
- invalid methods and lowercase methods;
- invalid redirect status;
- duplicate route path in the same server.

## Parser Responsibilities

What it does:
the parser converts tokens into typed config objects and reports errors with
enough context for the user to fix the file quickly.

Why it exists:
the config parser is the boundary between arbitrary text and the rest of the
server. It must protect runtime, route resolution, and CGI code from malformed
state.

Subject requirement supported:
configuration file startup, multiple listeners, per-route behavior, default error
pages, max body size, uploads, and CGI.

Responsibilities:
- open and read the selected config file;
- tokenize text with line and column tracking;
- parse block structure;
- parse directive arguments into typed values;
- validate directive scope and arity;
- normalize paths, route prefixes, methods, sizes, and status codes;
- resolve inheritance into final effective route settings;
- return either a complete config object or a clear error.

Important invariants:
- parser does not open sockets;
- parser does not execute CGI;
- parser does not serve files;
- parser does not silently repair invalid input;
- parser output must be independent of temporary token storage lifetime.

Common failure modes:
- mixing parsing with runtime side effects;
- validating only syntax but not directive semantics;
- storing references to temporary strings;
- continuing after an error and producing misleading follow-up errors;
- accepting partial configs.

How to test it:
- parser unit tests with in-memory config strings if available;
- file-based tests for missing file, empty file, valid config, invalid syntax,
  and invalid semantics;
- assertions on normalized output values;
- assertions on line-aware error messages;
- startup test proving invalid config exits before runtime initialization.

## Evaluation Checklist

What it does:
this checklist maps the grammar to peer-evaluation questions and demo configs.

Why it exists:
the project must provide configuration files and default files that demonstrate
every mandatory feature.

Subject requirement supported:
all mandatory configuration-file requirements in IV.3.

Checklist:
- [ ] config argument works;
- [ ] default config path works;
- [ ] at least two `listen` interface:port pairs are demonstrated;
- [ ] static site served from `root`;
- [ ] custom `404` error page demonstrated;
- [ ] `client_max_body_size` rejects an oversized body;
- [ ] route allows only configured methods;
- [ ] redirect route returns configured status and target;
- [ ] route root maps URL suffix correctly;
- [ ] autoindex enabled route lists a directory;
- [ ] autoindex disabled route does not list a directory;
- [ ] index file served for directory request;
- [ ] upload route stores client upload in configured directory;
- [ ] DELETE works on an allowed route;
- [ ] CGI runs for a configured extension;
- [ ] invalid config fails startup cleanly.

Important invariants:
- every checked behavior must have a concrete config and file fixture;
- demo configs should be small enough to explain during evaluation;
- bonus-only behavior must not be required to pass mandatory checks.

Common failure modes:
- having parser support without demo files;
- having demo files for behavior the parser does not validate;
- relying only on browser testing for invalid configs;
- forgetting to document final directive names after implementation changes.

How to test it:
- keep `configs/default.conf` as the main valid demo config;
- keep invalid configs under `tests/` or a documented config fixture directory;
- run manual HTTP requests for each checklist item;
- compare selected behavior with NGINX where the subject suggests it.
