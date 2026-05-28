#!/usr/bin/env python3
"""Small test runner for the 42 webserv project."""

from __future__ import print_function

import argparse
import os
import subprocess
import sys


ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SECTIONS = ("all", "build", "config", "runtime", "http", "cgi", "stress")

SECTION_TITLES = {
    "all": "All tests",
    "build": "Build",
    "config": "Config foundation",
    "runtime": "Runtime tests",
    "http": "HTTP",
    "cgi": "CGI",
    "stress": "Stress",
    "planned": "Planned",
}


class Colors(object):
    def __init__(self, enabled):
        self.enabled = enabled

    def paint(self, text, code):
        if not self.enabled:
            return text
        return "\033[" + code + "m" + text + "\033[0m"

    def green(self, text):
        return self.paint(text, "32")

    def red(self, text):
        return self.paint(text, "31")

    def yellow(self, text):
        return self.paint(text, "33")

    def cyan(self, text):
        return self.paint(text, "36")

    def bold(self, text):
        return self.paint(text, "1")


class TestCase(object):
    def __init__(self, section, name, command=None, description="",
                 skip_reason="", validator=None, timeout=None,
                 timeout_ok=False):
        self.section = section
        self.name = name
        self.command = command
        self.description = description
        self.skip_reason = skip_reason
        self.validator = validator
        self.timeout = timeout
        self.timeout_ok = timeout_ok


class TestResult(object):
    def __init__(self, test, status, exit_code=0, stdout="", stderr="",
                 reason=""):
        self.test = test
        self.status = status
        self.exit_code = exit_code
        self.stdout = stdout
        self.stderr = stderr
        self.reason = reason


def no_relink_validator(stdout, stderr):
    output = (stdout + "\n" + stderr).strip()
    if output == "":
        return ""
    if "Nothing to be done" in output or "is up to date" in output:
        return ""
    compile_markers = ("c++ ", " cc ", " gcc ", " clang", "mkdir -p")
    for marker in compile_markers:
        if marker in output:
            return "Makefile rebuilt or relinked on a second make"
    return "Second make produced unexpected output"


def build_tests():
    return [
        TestCase("build", "Clean previous build artifacts",
                 ["make", "fclean"],
                 "Remove object files, webserv, and test binaries"),
        TestCase("build", "Compile project with C++98 flags",
                 ["make"], "Build the main webserv binary"),
        TestCase("build", "Verify Makefile does not relink",
                 ["make"], "A second make should not rebuild anything",
                 validator=no_relink_validator),
        TestCase("build", "Start webserv with default config",
                 ["./webserv", "configs/default.conf"],
                 "Smoke test startup with configs/default.conf", timeout=1,
                 timeout_ok=True),
    ]


def config_tests():
    return [
        TestCase(
            "config",
            "HttpMethod, ConfigToken, ConfigException, tokenizer, and "
            "config model tests",
            ["make", "test_config_internal"],
            "Run the active config foundation C++ test binary",
        )
    ]


def runtime_tests():
    return [
        TestCase(
            "runtime",
            "ListenerManager, EventLoop, ClientConnection, ClientManager, "
            "ClientIo, cleanup, and dummy response runtime tests",
            ["make", "test_runtime_internal"],
            "Run the active runtime C++ test binaries",
        )
    ]


def planned_tests():
    return [
        TestCase("planned", "HTTP section",
                 skip_reason="no active HTTP parser/server behavior tests yet"),
        TestCase("planned", "CGI section",
                 skip_reason="no active CGI tests yet"),
        TestCase("planned", "Stress section",
                 skip_reason="no active stress tests yet"),
    ]


def tests_for_section(section):
    if section == "all":
        return build_tests() + config_tests() + runtime_tests() + planned_tests()
    if section == "build":
        return build_tests()
    if section == "config":
        return config_tests()
    if section == "runtime":
        return runtime_tests()
    if section == "http":
        return planned_tests()[0:1]
    if section == "cgi":
        return planned_tests()[1:2]
    if section == "stress":
        return planned_tests()[2:3]
    return []


def command_text(command):
    if command is None:
        return "<none>"
    return " ".join(command)


def run_test(test, colors, verbose):
    if test.skip_reason:
        result = TestResult(test, "skipped", reason=test.skip_reason)
        print_result(result, colors)
        return result
    print_start(test, colors, verbose)
    result = execute_test(test)
    print_result(result, colors)
    return result


def execute_test(test):
    process = subprocess.Popen(
        test.command,
        cwd=ROOT_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    try:
        stdout, stderr = process.communicate(timeout=test.timeout)
    except subprocess.TimeoutExpired:
        stdout, stderr = stop_process(process)
        return timeout_result(test, stdout, stderr)
    except KeyboardInterrupt:
        stdout, stderr = stop_process(process)
        return TestResult(test, "failed", exit_code="interrupted",
                          stdout=stdout, stderr=stderr,
                          reason="interrupted by user")
    result = command_result(test, process.returncode, stdout, stderr)
    if test.validator is not None and result.status == "passed":
        reason = test.validator(stdout, stderr)
        if reason:
            result.status = "failed"
            result.reason = reason
    return result


def stop_process(process):
    process.terminate()
    try:
        return process.communicate(timeout=2)
    except subprocess.TimeoutExpired:
        process.kill()
        return process.communicate()


def command_result(test, exit_code, stdout, stderr):
    if exit_code == 0:
        return TestResult(test, "passed", stdout=stdout, stderr=stderr)
    return TestResult(test, "failed", exit_code=exit_code, stdout=stdout,
                      stderr=stderr, reason="command exited with nonzero status")


def timeout_result(test, stdout, stderr):
    if test.timeout_ok:
        return TestResult(test, "passed", exit_code="timeout", stdout=stdout,
                          stderr=stderr)
    return TestResult(test, "failed", exit_code="timeout", stdout=stdout,
                      stderr=stderr, reason="command timed out")


def run_tests(section, colors, verbose):
    results = []
    current_section = None
    for test in tests_for_section(section):
        if test.section != current_section:
            current_section = test.section
            print_section(current_section, colors)
        results.append(run_test(test, colors, verbose))
        if has_failed(results):
            break
    return results


def has_failed(results):
    for result in results:
        if result.status == "failed":
            return True
    return False


def count_status(results, status):
    total = 0
    for result in results:
        if result.status == status:
            total += 1
    return total


def print_header(title, colors):
    print("")
    print(colors.bold(colors.cyan("== " + title + " ==")))


def print_section(section, colors):
    print("")
    print(colors.bold("[" + SECTION_TITLES[section] + "]"))


def print_start(test, colors, verbose):
    print("  %s   %s" % (colors.cyan("RUN"), test.name))
    if verbose:
        if test.description:
            print("        detail: %s" % test.description)
        print("        command: %s" % command_text(test.command))
    sys.stdout.flush()


def print_result(result, colors):
    if result.status == "passed":
        print("  %s  %s" % (colors.green("PASS"), result.test.name))
    elif result.status == "skipped":
        print("  %s  %s" % (colors.yellow("SKIP"), result.test.name))
        print("        reason: %s" % result.reason)
    else:
        print("  %s  %s" % (colors.red("FAIL"), result.test.name))
        print_failure_detail(result)


def print_failure_detail(result):
    print("        section: %s" % SECTION_TITLES[result.test.section])
    print("        test: %s" % result.test.name)
    print("        command: %s" % command_text(result.test.command))
    print("        exit code: %s" % result.exit_code)
    if result.reason:
        print("        likely reason: %s" % result.reason)
    print_tail("stdout", result.stdout)
    print_tail("stderr", result.stderr)


def print_tail(label, text):
    tail = tail_text(text, 12)
    if not tail:
        return
    print("        %s tail:" % label)
    print(indent(tail))


def tail_text(text, max_lines):
    lines = text.splitlines()
    if len(lines) <= max_lines:
        return "\n".join(lines)
    return "\n".join(lines[-max_lines:])


def indent(text):
    return "\n".join("          " + line for line in text.splitlines())


def print_summary(results, colors):
    passed = count_status(results, "passed")
    failed = count_status(results, "failed")
    skipped = count_status(results, "skipped")
    summary = "Summary: %d passed, %d failed, %d skipped" % (
        passed, failed, skipped)
    if failed:
        print("")
        print(colors.red(summary))
    else:
        print("")
        print(colors.green(summary))


def list_sections():
    print("Available test sections:")
    print("  all      build, config foundation, runtime tests, planned")
    print("  build    clean build, compile, no-relink, startup smoke")
    print("  config   active config/parser/model foundation tests")
    print("  runtime  active listener, event loop, client I/O, cleanup, "
          "and dummy response tests")
    print("  http     planned; currently skipped")
    print("  cgi      planned; currently skipped")
    print("  stress   planned; currently skipped")


def normalize_choice(choice):
    text = choice.strip().lower()
    numeric = {
        "1": "all",
        "2": "build",
        "3": "config",
        "4": "runtime",
        "5": "http",
        "6": "cgi",
        "7": "stress",
        "8": "list",
        "9": "quit",
        "q": "quit",
        "quit": "quit",
        "exit": "quit",
    }
    if text in numeric:
        return numeric[text]
    if text in SECTIONS or text == "list":
        return text
    return ""


def print_menu(colors):
    print_header("webserv test menu", colors)
    print("  1. All tests")
    print("  2. Build")
    print("  3. Config foundation")
    print("  4. Runtime tests")
    print("  5. HTTP (planned)")
    print("  6. CGI (planned)")
    print("  7. Stress (planned)")
    print("  8. List sections")
    print("  9. Quit")


def interactive_menu(colors, verbose):
    while True:
        print_menu(colors)
        try:
            choice = input("> ")
        except KeyboardInterrupt:
            print("")
            print("Interrupted. Returning to shell.")
            return 0
        except EOFError:
            print("")
            print("EOF received. Returning to shell.")
            return 0
        section = normalize_choice(choice)
        if choice.strip() == "":
            print("Choose a number or section name.")
            continue
        if section == "":
            print("Unknown choice: %s" % choice.strip())
            continue
        if section == "quit":
            return 0
        if section == "list":
            print("")
            list_sections()
            continue
        run_selected_section(section, colors, verbose)


def run_selected_section(section, colors, verbose):
    print_header(SECTION_TITLES[section], colors)
    results = run_tests(section, colors, verbose)
    print_summary(results, colors)
    return 1 if has_failed(results) else 0


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Run webserv tests")
    parser.add_argument("section", nargs="?", choices=SECTIONS + ("list",))
    parser.add_argument("--no-color", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)
    colors = Colors(sys.stdout.isatty() and not args.no_color)
    if args.section is None:
        return interactive_menu(colors, args.verbose)
    if args.section == "list":
        list_sections()
        return 0
    return run_selected_section(args.section, colors, args.verbose)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
