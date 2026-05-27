# Stress Tests

Status: Planned

Stress tests will be added after the persistent runtime loop, HTTP handling, and
cleanup paths exist.

Planned coverage:

- `siege` availability above 99.5%;
- no memory growth across repeated traffic;
- no hanging connections;
- clean behavior under slow or disconnected clients;
- repeated static and dynamic requests.

`python3 tests/run.py stress` currently reports this section as skipped.

