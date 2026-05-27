# HTTP Tests

Status: Planned

HTTP behavior tests will be added after request parsing, routing, and response
generation are implemented.

Planned coverage:

- `GET`;
- `POST`;
- `DELETE`;
- unknown methods without crashing;
- correct status codes;
- static file serving;
- wrong URL handling;
- autoindex;
- redirects;
- upload and retrieve file behavior.

`python3 tests/run.py http` currently reports this section as skipped.

