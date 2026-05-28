NAME	:= webserv

CXX		:= c++
CXXFLAGS:= -Wall -Wextra -Werror -std=c++98
INC		:= -Iinclude

SRC_DIR	:= src
OBJ_DIR	:= obj

COMMON_SRCS	:= $(SRC_DIR)/ClientConnection.cpp \
		   $(SRC_DIR)/ClientIo.cpp \
		   $(SRC_DIR)/ClientManager.cpp \
		   $(SRC_DIR)/DummyResponseRuntime.cpp \
		   $(SRC_DIR)/EventLoop.cpp \
		   $(SRC_DIR)/ListenerConfig.cpp \
		   $(SRC_DIR)/ListenerManager.cpp
SRCS	:= $(SRC_DIR)/main.cpp $(COMMON_SRCS)
OBJS	:= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

TEST_LISTENER	:= tests/test_listener_manager
TEST_CLIENT	:= tests/test_client_connection
TEST_CLIENT_CLEANUP	:= tests/test_client_cleanup
TEST_CLIENT_IO	:= tests/test_client_io
TEST_CLIENT_MANAGER	:= tests/test_client_manager
TEST_DUMMY_RESPONSE	:= tests/test_dummy_response_runtime
TEST_EVENT_LOOP	:= tests/test_event_loop

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TEST_LISTENER): tests/test_listener_manager.cpp $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(INC) tests/test_listener_manager.cpp $(COMMON_SRCS) -o $(TEST_LISTENER)

$(TEST_CLIENT): tests/test_client_connection.cpp $(SRC_DIR)/ClientConnection.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/test_client_connection.cpp $(SRC_DIR)/ClientConnection.cpp -o $(TEST_CLIENT)

$(TEST_CLIENT_CLEANUP): tests/test_client_cleanup.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/ClientManager.cpp $(SRC_DIR)/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/test_client_cleanup.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/ClientManager.cpp $(SRC_DIR)/EventLoop.cpp -o $(TEST_CLIENT_CLEANUP)

$(TEST_CLIENT_IO): tests/test_client_io.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/ClientIo.cpp $(SRC_DIR)/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/test_client_io.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/ClientIo.cpp $(SRC_DIR)/EventLoop.cpp -o $(TEST_CLIENT_IO)

$(TEST_CLIENT_MANAGER): tests/test_client_manager.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/ClientManager.cpp $(SRC_DIR)/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/test_client_manager.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/ClientManager.cpp $(SRC_DIR)/EventLoop.cpp -o $(TEST_CLIENT_MANAGER)

$(TEST_DUMMY_RESPONSE): tests/test_dummy_response_runtime.cpp $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(INC) tests/test_dummy_response_runtime.cpp $(COMMON_SRCS) -o $(TEST_DUMMY_RESPONSE)

$(TEST_EVENT_LOOP): tests/test_event_loop.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/EventLoop.cpp
	$(CXX) $(CXXFLAGS) $(INC) tests/test_event_loop.cpp $(SRC_DIR)/ClientConnection.cpp $(SRC_DIR)/EventLoop.cpp -o $(TEST_EVENT_LOOP)

test: $(TEST_LISTENER) $(TEST_CLIENT) $(TEST_CLIENT_CLEANUP) $(TEST_CLIENT_IO) $(TEST_CLIENT_MANAGER) $(TEST_DUMMY_RESPONSE) $(TEST_EVENT_LOOP)
	./$(TEST_LISTENER)
	./$(TEST_CLIENT)
	./$(TEST_CLIENT_CLEANUP)
	./$(TEST_CLIENT_IO)
	./$(TEST_CLIENT_MANAGER)
	./$(TEST_DUMMY_RESPONSE)
	./$(TEST_EVENT_LOOP)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_LISTENER) $(TEST_CLIENT) $(TEST_CLIENT_CLEANUP) $(TEST_CLIENT_IO) $(TEST_CLIENT_MANAGER) $(TEST_DUMMY_RESPONSE) $(TEST_EVENT_LOOP)

re: fclean all

.PHONY: all test clean fclean re
