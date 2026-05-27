# Config Tests

Status: Implemented

This directory stores config fixtures for upcoming semantic parser tests.
It also contains active C++ tests for implemented config/parser/model
foundation behavior.

Current parser status:

- `ConfigTokenizer` exists.
- `ConfigParser` wires file content through tokenization into parser state.
- Full semantic parsing into `ServerConfig`, `RouteConfig`, and `ListenConfig`
  is not implemented yet.

Active tests:

- `test_config_foundation.cpp`

Run with:

```sh
python3 tests/run.py config
```

or:

```sh
make test_config
```

Status: Planned

Future config coverage:

- multiple ports;
- multiple hostnames;
- error pages;
- body-size limits;
- routes;
- index behavior;
- allowed methods;
- invalid syntax and invalid directive scope.
