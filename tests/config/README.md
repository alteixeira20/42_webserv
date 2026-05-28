# Config Tests

Status: Implemented

This directory contains active C++ tests and fixtures for the implemented config
parser, validator, and resolver.

Current coverage in `test_config_foundation.cpp`:

- config model value objects and tokenizer basics;
- semantic parser output for `Config`, `ServerConfig`, `ListenConfig`, and
  `RouteConfig`;
- valid `listen` values, server directives, multi-status `error_page`, and
  `location` blocks;
- route directives: `root`, `index`, `autoindex`, `allowed_methods`,
  `redirect`, `upload_dir`, and `cgi`;
- parser rejections for invalid listens, ports, methods, booleans, redirect
  statuses, and body sizes;
- validator rejections for empty configs, missing listen directives, duplicate
  listens, duplicate default servers, duplicate server names, duplicate route
  paths, and non-absolute route paths;
- resolver selection for server_name matches, endpoint defaults, longest route
  prefix, and segment-boundary behavior.

Fixtures:

- `valid_minimal.conf`: smallest valid server config.
- `valid_comments.conf`: valid config with comments and whitespace.
- `valid_full.conf`: valid config covering implemented directives.
- `invalid_missing_semicolon.conf`: syntax error fixture.
- `invalid_route_path.conf`: semantic route path error fixture.

Run with:

```sh
python3 tests/run.py config
```

or:

```sh
make test_config_internal
```

Use `python3 tests/run.py all --no-color` for the full active suite.
