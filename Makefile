NAME		:= webserv

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98
INC			:= -Iinclude
GIT			:= git
DIFF_OUT	:= .git/diff_report.txt
DIFF_FILES	:= .gitignore Makefile README.md configs docs include src tests

SRC_DIR		:= src
OBJ_DIR		:= obj

LINT_FILES	:= .gitignore Makefile README.md configs docs include src tests
CODE_FILES	:= $(shell find include src tests -type f \( -name '*.hpp' -o -name '*.h' -o -name '*.cpp' -o -name '*.c' \))
TEST_BINS	:= tests/config/test_config_foundation \
			   tests/runtime/test_client_connection \
			   tests/runtime/test_client_cleanup \
			   tests/runtime/test_client_io \
			   tests/runtime/test_client_manager \
			   tests/runtime/test_dummy_response_runtime \
			   tests/runtime/test_event_loop \
			   tests/runtime/test_listener_manager

COMMON_SRCS	:= \
	$(SRC_DIR)/config/Config.cpp \
	$(SRC_DIR)/config/ConfigException.cpp \
	$(SRC_DIR)/config/ConfigParser.cpp \
	$(SRC_DIR)/config/ConfigParserServer.cpp \
	$(SRC_DIR)/config/ConfigParserRoute.cpp \
	$(SRC_DIR)/config/ConfigParserValues.cpp \
	$(SRC_DIR)/config/ConfigResolver.cpp \
	$(SRC_DIR)/config/ConfigToken.cpp \
	$(SRC_DIR)/config/ConfigTokenizer.cpp \
	$(SRC_DIR)/config/ConfigValidator.cpp \
	$(SRC_DIR)/config/ListenConfig.cpp \
	$(SRC_DIR)/config/RouteConfig.cpp \
	$(SRC_DIR)/config/ServerConfig.cpp \
	$(SRC_DIR)/http/HttpMethod.cpp \
	$(SRC_DIR)/runtime/ClientConnection.cpp \
	$(SRC_DIR)/runtime/ClientIo.cpp \
	$(SRC_DIR)/runtime/ClientManager.cpp \
	$(SRC_DIR)/runtime/DummyResponseRuntime.cpp \
	$(SRC_DIR)/runtime/EventLoop.cpp \
	$(SRC_DIR)/runtime/ListenerManager.cpp

SRCS		:= \
	$(SRC_DIR)/main.cpp \
	$(COMMON_SRCS)

OBJS		:= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

TEST_LISTENER	:= tests/runtime/test_listener_manager
TEST_CLIENT		:= tests/runtime/test_client_connection
TEST_CLIENT_CLEANUP	:= tests/runtime/test_client_cleanup
TEST_CLIENT_IO	:= tests/runtime/test_client_io
TEST_CLIENT_MANAGER	:= tests/runtime/test_client_manager
TEST_DUMMY_RESPONSE	:= tests/runtime/test_dummy_response_runtime
TEST_EVENT_LOOP	:= tests/runtime/test_event_loop
TEST_CONFIG		:= tests/config/test_config_foundation

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

$(TEST_LISTENER): tests/runtime/test_listener_manager.cpp $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_listener_manager.cpp $(COMMON_SRCS) -o $(TEST_LISTENER)

$(TEST_CLIENT): tests/runtime/test_client_connection.cpp $(SRC_DIR)/runtime/ClientConnection.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_client_connection.cpp $(SRC_DIR)/runtime/ClientConnection.cpp -o $(TEST_CLIENT)

$(TEST_CLIENT_CLEANUP): tests/runtime/test_client_cleanup.cpp $(SRC_DIR)/runtime/ClientConnection.cpp $(SRC_DIR)/runtime/ClientManager.cpp $(SRC_DIR)/runtime/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_client_cleanup.cpp $(SRC_DIR)/runtime/ClientConnection.cpp $(SRC_DIR)/runtime/ClientManager.cpp $(SRC_DIR)/runtime/EventLoop.cpp -o $(TEST_CLIENT_CLEANUP)

$(TEST_CLIENT_IO): tests/runtime/test_client_io.cpp $(SRC_DIR)/runtime/ClientConnection.cpp $(SRC_DIR)/runtime/ClientIo.cpp $(SRC_DIR)/runtime/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_client_io.cpp $(SRC_DIR)/runtime/ClientConnection.cpp $(SRC_DIR)/runtime/ClientIo.cpp $(SRC_DIR)/runtime/EventLoop.cpp -o $(TEST_CLIENT_IO)

$(TEST_CLIENT_MANAGER): tests/runtime/test_client_manager.cpp $(SRC_DIR)/runtime/ClientConnection.cpp $(SRC_DIR)/runtime/ClientManager.cpp $(SRC_DIR)/runtime/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_client_manager.cpp $(SRC_DIR)/runtime/ClientConnection.cpp $(SRC_DIR)/runtime/ClientManager.cpp $(SRC_DIR)/runtime/EventLoop.cpp -o $(TEST_CLIENT_MANAGER)

$(TEST_DUMMY_RESPONSE): tests/runtime/test_dummy_response_runtime.cpp $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_dummy_response_runtime.cpp $(COMMON_SRCS) -o $(TEST_DUMMY_RESPONSE)

$(TEST_EVENT_LOOP): tests/runtime/test_event_loop.cpp $(SRC_DIR)/runtime/ClientConnection.cpp $(SRC_DIR)/runtime/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_event_loop.cpp $(SRC_DIR)/runtime/ClientConnection.cpp $(SRC_DIR)/runtime/EventLoop.cpp -o $(TEST_EVENT_LOOP)

test_config_internal: $(TEST_CONFIG)
	./$(TEST_CONFIG)

test_runtime_internal: $(TEST_LISTENER) $(TEST_CLIENT) $(TEST_CLIENT_CLEANUP) $(TEST_CLIENT_IO) $(TEST_CLIENT_MANAGER) $(TEST_DUMMY_RESPONSE) $(TEST_EVENT_LOOP)
	./$(TEST_LISTENER)
	./$(TEST_CLIENT)
	./$(TEST_CLIENT_CLEANUP)
	./$(TEST_CLIENT_IO)
	./$(TEST_CLIENT_MANAGER)
	./$(TEST_DUMMY_RESPONSE)
	./$(TEST_EVENT_LOOP)

test:
	python3 tests/run.py

$(TEST_CONFIG): tests/config/test_config_foundation.cpp $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(INC) tests/config/test_config_foundation.cpp $(COMMON_SRCS) -o $(TEST_CONFIG)

diff:
	@mkdir -p "$$(dirname "$(DIFF_OUT)")"
	@rm -f "$(DIFF_OUT)"
	@{ \
		git status --short; \
		git diff --stat -- $(DIFF_FILES); \
		git diff -- $(DIFF_FILES); \
		git ls-files --others --exclude-standard $(DIFF_FILES) | while read file; do \
			echo ""; \
			echo "Untracked: $$file"; \
			git diff --no-index /dev/null "$$file" || true; \
		done; \
	} | tee "$(DIFF_OUT)"
	@echo ""
	@printf "Copy diff output to clipboard? [y/N] "; \
	read answer; \
	if [ "$$answer" = "y" ] || [ "$$answer" = "Y" ] || [ "$$answer" = "yes" ] || [ "$$answer" = "YES" ]; then \
		if command -v wl-copy >/dev/null 2>&1; then \
			cat "$(DIFF_OUT)" | wl-copy; \
			echo "Copied diff output to clipboard with wl-copy."; \
		elif command -v xclip >/dev/null 2>&1; then \
			cat "$(DIFF_OUT)" | xclip -selection clipboard; \
			echo "Copied diff output to clipboard with xclip."; \
		elif command -v xsel >/dev/null 2>&1; then \
			cat "$(DIFF_OUT)" | xsel --clipboard --input; \
			echo "Copied diff output to clipboard with xsel."; \
		elif command -v pbcopy >/dev/null 2>&1; then \
			cat "$(DIFF_OUT)" | pbcopy; \
			echo "Copied diff output to clipboard with pbcopy."; \
		else \
			echo "Clipboard tool not found. Diff output saved at $(DIFF_OUT)."; \
			echo "Install one of: wl-clipboard, xclip, xsel, or use macOS pbcopy."; \
		fi; \
	else \
		echo "Diff output saved at $(DIFF_OUT)."; \
	fi

lint:
	@status=0; \
	echo "== lint: git whitespace errors =="; \
	echo "checks: trailing whitespace, bad conflict markers, whitespace errors in current diff"; \
	git diff --check -- $(LINT_FILES) || status=1; \
	echo ""; \
	echo "== lint: final newline =="; \
	echo "checks: every C/C++ source/header file ends with a newline"; \
	for file in $(CODE_FILES); do \
		if [ -s "$$file" ] && [ "$$(tail -c 1 "$$file" | wc -l)" -eq 0 ]; then \
			echo "missing final newline: $$file"; \
			status=1; \
		fi; \
	done; \
	echo ""; \
	echo "== lint: excessive blank lines =="; \
	echo "checks: more than two consecutive blank lines in C/C++ files"; \
	for file in $(CODE_FILES); do \
		awk 'BEGIN {blank=0; found=0} \
			/^$$/ {blank++} \
			!/^[[:space:]]*$$/ {blank=0} \
			blank > 2 {print FILENAME ":" FNR ": more than two consecutive blank lines"; found=1} \
			END {exit found}' "$$file" || status=1; \
	done; \
	echo ""; \
	echo "== lint: generated test binaries =="; \
	echo "checks: generated test executables are not tracked by git"; \
	if git ls-files $(TEST_BINS) | grep .; then \
		echo "generated test binary is tracked"; \
		status=1; \
	fi; \
	echo ""; \
	echo "== lint: conflict markers =="; \
	echo "checks: no unresolved merge conflict markers in project files"; \
	if grep -RInE '^(<<<<<<<|=======|>>>>>>>)' $(LINT_FILES) >/tmp/webserv_lint_conflicts.txt 2>/dev/null; then \
		cat /tmp/webserv_lint_conflicts.txt; \
		rm -f /tmp/webserv_lint_conflicts.txt; \
		status=1; \
	else \
		rm -f /tmp/webserv_lint_conflicts.txt; \
	fi; \
	echo ""; \
	if [ $$status -eq 0 ]; then \
		echo "lint: OK"; \
	else \
		echo "lint: FAILED"; \
	fi; \
	exit $$status

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_LISTENER) $(TEST_CLIENT) $(TEST_CLIENT_CLEANUP) $(TEST_CLIENT_IO) $(TEST_CLIENT_MANAGER) $(TEST_DUMMY_RESPONSE) $(TEST_EVENT_LOOP) $(TEST_CONFIG)

re: fclean all

.PHONY: all test test_config_internal test_runtime_internal diff lint clean fclean re
