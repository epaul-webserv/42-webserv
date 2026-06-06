NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -g -std=c++98

.PHONY: all
all: $(NAME)

INCLUDE_DIR = include
SRC_DIR = src
BUILD_DIR = build

SRCS = main.cpp common_header.cpp

OBJS = $(addprefix $(BUILD_DIR)/,$(SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) -MMD -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(DEPS)

.PHONY: fclean
fclean: clean
	rm -f $(NAME)

.PHONY: re
re:
	$(MAKE) fclean
	$(MAKE) all

-include $(DEPS)
