NAME		:= webserv

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98
INC			:= -Iinclude

SRC_DIR		:= src
OBJ_DIR		:= obj

COMMON_SRCS	:= \
	$(SRC_DIR)/ClientConnection.cpp \
	$(SRC_DIR)/ClientManager.cpp \
	$(SRC_DIR)/EventLoop.cpp \
	$(SRC_DIR)/ListenerManager.cpp \
	$(SRC_DIR)/config/Config.cpp \
	$(SRC_DIR)/config/ConfigException.cpp \
	$(SRC_DIR)/config/ConfigParser.cpp \
	$(SRC_DIR)/config/ConfigToken.cpp \
	$(SRC_DIR)/config/ConfigTokenizer.cpp \
	$(SRC_DIR)/config/ListenConfig.cpp \
	$(SRC_DIR)/config/RouteConfig.cpp \
	$(SRC_DIR)/config/ServerConfig.cpp \
	$(SRC_DIR)/http/HttpMethod.cpp

SRCS		:= \
	$(SRC_DIR)/main.cpp \
	$(COMMON_SRCS)

OBJS		:= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

TEST_LISTENER		:= tests/runtime/test_listener_manager
TEST_CLIENT		:= tests/runtime/test_client_connection
TEST_CLIENT_MANAGER	:= tests/runtime/test_client_manager
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

$(TEST_CLIENT): tests/runtime/test_client_connection.cpp $(SRC_DIR)/ClientConnection.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_client_connection.cpp $(SRC_DIR)/ClientConnection.cpp -o $(TEST_CLIENT)

$(TEST_CLIENT_MANAGER): tests/runtime/test_client_manager.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/ClientManager.cpp $(SRC_DIR)/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_client_manager.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/ClientManager.cpp $(SRC_DIR)/EventLoop.cpp -o $(TEST_CLIENT_MANAGER)

$(TEST_EVENT_LOOP): tests/runtime/test_event_loop.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/runtime/test_event_loop.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/EventLoop.cpp -o $(TEST_EVENT_LOOP)

test_config_internal: $(TEST_CONFIG)
	./$(TEST_CONFIG)

test_runtime_internal: $(TEST_LISTENER) $(TEST_CLIENT) $(TEST_CLIENT_MANAGER) $(TEST_EVENT_LOOP)
	./$(TEST_LISTENER)
	./$(TEST_CLIENT)
	./$(TEST_CLIENT_MANAGER)
	./$(TEST_EVENT_LOOP)

test:
	python3 tests/run.py

$(TEST_CONFIG): tests/config/test_config_foundation.cpp $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(INC) tests/config/test_config_foundation.cpp $(COMMON_SRCS) -o $(TEST_CONFIG)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_LISTENER) $(TEST_CLIENT) $(TEST_CLIENT_MANAGER) $(TEST_EVENT_LOOP) $(TEST_CONFIG)

re: fclean all

.PHONY: all test test_config_internal test_runtime_internal clean fclean re
