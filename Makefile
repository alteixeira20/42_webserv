NAME		:= webserv

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98
INC			:= -Iinclude

SRC_DIR		:= src
OBJ_DIR		:= obj

SRCS		:= \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/config/Config.cpp \
	$(SRC_DIR)/config/ConfigParser.cpp \
	$(SRC_DIR)/config/ConfigToken.cpp \
	$(SRC_DIR)/config/ConfigTokenizer.cpp \
	$(SRC_DIR)/config/RouteConfig.cpp \
	$(SRC_DIR)/config/ServerConfig.cpp \
	$(SRC_DIR)/http/HttpMethod.cpp

OBJS		:= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
