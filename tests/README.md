# Test Suite

This directory contains the project test-suite foundation.

## Running Tests

`tests/run.py` is the user-facing test entrypoint. `make test` opens the same
interactive menu:

```sh
make test
python3 tests/run.py
```

Direct command-line sections are also available:

```sh
python3 tests/run.py list
python3 tests/run.py all
python3 tests/run.py build
python3 tests/run.py config
python3 tests/run.py runtime
python3 tests/run.py http
python3 tests/run.py cgi
python3 tests/run.py stress
python3 tests/run.py all --no-color
python3 tests/run.py all --verbose
```

## Reporting

The runner groups output by section and prints human-readable test names instead
of raw commands. Passing tests stay compact: stdout and stderr are hidden unless
something fails.

On failure, the report includes the section, test name, exact command, exit code,
likely reason when known, and stdout/stderr tails. Use `--verbose` to show the
command under each test while it runs.

## Sections

Status: Implemented

- `build`: runs `make fclean`, `make`, a no-relink `make`, and a startup
  smoke test with `configs/default.conf`.
- `config`: runs active C++ tests for implemented config/parser/model
  foundation behavior through the internal Makefile target.
- `runtime`: runs the existing C++ runtime foundation tests through
  the internal Makefile target.

Status: Planned

- `http`: HTTP request/response behavior.
- `cgi`: CGI execution behavior.
- `stress`: availability, hanging connection, and memory-growth checks.

Planned sections are reported as one skipped section each. They are not expanded
into fake tests.

## Current Limitations

The suite does not hide incomplete server behavior. Planned sections are skipped
until the corresponding implementation exists. Do not add fake passing tests for
unimplemented features.
