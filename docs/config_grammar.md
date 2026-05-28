# Config Grammar

This document describes the configuration language implemented on
`feature/config-parser`.

The config layer is split into three responsibilities:

- `ConfigParser` reads files or strings, tokenizes them, parses `server` and
  `location` blocks, and converts directive values into typed config objects.
- `ConfigValidator` checks semantic rules that need whole-config context, such
  as required listeners and duplicate virtual hosts or routes.
- `ConfigResolver` is runtime-facing lookup logic. It chooses a server for a
  listen endpoint plus Host value, and chooses a route with longest prefix
  matching.

## File Shape

A config file contains one or more `server` blocks:

```nginx
config        = server_block+
server_block  = "server" "{" server_directive* location_block* "}"
location_block = "location" path "{" route_directive* "}"
directive     = name argument* ";"
comment       = "#" text_until_end_of_line
```

Rules:

- The file must contain at least one `server` block.
- Every directive must end with `;`.
- Braces must be balanced.
- Unknown directives are rejected.
- `location` blocks are only valid inside a `server` block.
- `location` paths must start with `/`.
- Route paths are plain URL prefixes, not regular expressions.

## Lexical Rules

- Whitespace separates tokens.
- `#` starts a comment until the end of the line.
- `{`, `}`, and `;` are standalone tokens.
- Directive names are case-sensitive.
- Values are unquoted words. Quoted strings and spaces inside values are not
  currently supported.

## Supported Directives

Server directives:

| Directive | Arguments | Required | Meaning |
| --- | --- | --- | --- |
| `listen` | `port` or `host:port` | yes, one or more | Endpoint to bind. Port-only listens use `0.0.0.0`. |
| `server_name` | name... | no | Virtual-host names for Host matching. |
| `root` | path | no | Server-level default document root. |
| `index` | filename | no | Server-level default index filename. |
| `client_max_body_size` | positive integer bytes | no | Maximum request body size. |
| `error_page` | status... path | no | Map one or more 3xx-5xx statuses to one error page path. |

Route directives inside `location`:

| Directive | Arguments | Required | Meaning |
| --- | --- | --- | --- |
| `root` | path | no | Route-specific document root. |
| `index` | filename | no | Route-specific index filename. |
| `autoindex` | `on`, `off`, `true`, `false`, `1`, or `0` | no | Directory listing flag. |
| `allowed_methods` | method... | no | Accepted methods. Only `GET`, `POST`, and `DELETE` are valid. |
| `redirect` | 3xx_status target | no | Redirect response configuration. |
| `upload_dir` | path | no | Upload storage directory. |
| `cgi` | extension executable_path | no | CGI executable for an extension. |

## Validation Rules

The validator rejects:

- empty configs;
- any server without a `listen` directive;
- duplicate listen endpoints inside the same server;
- more than one unnamed default server for the same listen endpoint;
- duplicate `server_name` values on the same listen endpoint;
- duplicate `location` paths inside the same server;
- route paths that do not start with `/`.

Multiple servers may share the same listen endpoint when their server names are
unique. The first server for an endpoint is the default server when no Host value
matches a configured `server_name`.

## Resolver Rules

`ConfigResolver::findServer()` first filters servers by exact listen host and
port. It returns the first matching `server_name`; if no name matches, it returns
the first server configured on that endpoint.

`ConfigResolver::findRoute()` returns the longest matching route prefix. Segment
boundaries are respected, so `/img` matches `/img/logo.png` but not `/img2`.
The root route `/` matches every path.

## Example

```nginx
server {
    listen 127.0.0.1:8080;
    listen 8081;
    server_name localhost example.test;

    root www;
    index index.html;
    client_max_body_size 1000000;
    error_page 403 404 www/errors/4xx.html;
    error_page 500 www/errors/5xx.html;

    location / {
        root www;
        index index.html;
        allowed_methods GET POST DELETE;
        autoindex off;
    }

    location /uploads {
        root www/uploads;
        upload_dir uploads;
        allowed_methods GET POST DELETE;
        autoindex on;
    }

    location /old {
        redirect 301 /;
    }

    location /cgi-bin {
        root www/cgi-bin;
        allowed_methods GET POST;
        cgi .py /usr/bin/python3;
    }
}
```

## Current Limitations

- Body sizes are plain positive integer byte counts. Suffixes such as `k`, `M`,
  or `G` are not implemented.
- Values are unquoted single tokens.
- Host matching is exact string matching; wildcard names are not implemented.
- Listen matching is exact host and port matching; wildcard overlap such as
  `0.0.0.0:8080` versus `127.0.0.1:8080` is not collapsed.
- The parser validates config semantics but does not check filesystem existence
  or executable permissions.
- Server-level defaults are stored but route inheritance and response behavior
  are handled by later runtime/HTTP work.
