# HTTP Tests

Status: Active

The active HTTP section currently covers the standalone request parser and
response builder foundations. It does not exercise runtime integration,
routing, body parsing, CGI, uploads, or static serving.

Active coverage:

- request-line parsing from complete and split input;
- header parsing until the blank line;
- case-insensitive header lookup;
- header whitespace trimming;
- malformed request-line and header errors;
- oversized header section errors;
- parser reset behavior.
- response status line, header, body, and `Content-Length` serialization;
- explicit `Connection` serialization;
- required status reason phrases;
- minimal `ResponseBuilder` close-after-response defaults;
- default HTML error pages;
- custom error-page response bodies supplied by later routing/static code.

`python3 tests/run.py http` runs `make test_http_internal`.

Full HTTP server behavior remains planned for later slices.
