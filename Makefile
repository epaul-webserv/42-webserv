NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -g -std=c++98

.PHONY: all
all: $(NAME)

LIBASYNC_PREFIX = libasync

LIBCONTAINER_PREFIX = $(LIBASYNC_PREFIX)/libcontainer

-include $(LIBCONTAINER_PREFIX)/module.mk

-include $(LIBASYNC_PREFIX)/module.mk

INCLUDE_DIR = include
SRC_DIR = src
BUILD_DIR = build

SRCS = main.cpp common_header.cpp

OBJS = $(addprefix $(BUILD_DIR)/,$(SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d)

$(NAME): $(OBJS) $(LIBASYNC_DEST)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I$(LIBASYNC_INCLUDE_DIR) -I$(LIBCONTAINER_INCLUDE_DIR) -I $(INCLUDE_DIR) -MMD -c $< -o $@

.PHONY: clean
clean: libasync_clean libcontainer_clean
	rm -f $(OBJS) $(DEPS)

.PHONY: fclean
fclean: clean
	rm -f $(NAME)

.PHONY: re
re:
	$(MAKE) fclean
	$(MAKE) all

-include $(DEPS)
