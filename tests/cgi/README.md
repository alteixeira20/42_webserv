# CGI Tests

Status: Planned

CGI tests will be added after CGI process and pipe handling are implemented.

Planned coverage:

- GET CGI;
- POST CGI;
- correct CGI working directory;
- environment and body forwarding;
- CGI errors;
- infinite loop or timeout handling;
- readiness-aware CGI pipe reads and writes.

`python3 tests/run.py cgi` currently reports this section as skipped.

