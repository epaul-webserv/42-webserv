CXX = c++
CXXFLAGS += -g3 -Wall -Wextra -Werror -std=c++98

SRCS_DIR = srcs
SRCS = main.cpp \
		string_overload/string_overload.cpp \
		url_utils/file_name_utils.cpp \
		url_utils/url_decoder.cpp \
		url_utils/url_encoder.cpp

CLASS_DIR = class
CLASS = ABackend.cpp \
		Date.cpp \
		HTTPStream.cpp \
		Request.cpp \
		Router.cpp \
		RouterRuleset.cpp

OBJS_DIR = objs
OBJS = $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(SRCS)) $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(CLASS))

DEPS = $(patsubst %.cpp, $(OBJS_DIR)/%.d, $(SRCS)) $(patsubst %.cpp, $(OBJS_DIR)/%.d, $(CLASS))

DEPFLAGS = -MMD

INCLUDES = includes

NAME = webserv

all : $(NAME)

LIBASYNC_PREFIX = libasync

LIBCONTAINER_PREFIX = $(LIBASYNC_PREFIX)/libcontainer

-include $(LIBCONTAINER_PREFIX)/module.mk

-include $(LIBASYNC_PREFIX)/module.mk

.PHONY = $(NAME) all clean fclean re

$(OBJS_DIR)/%.o : $(SRCS_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -I$(INCLUDES) -I$(LIBASYNC_INCLUDE_DIR) -I$(LIBCONTAINER_INCLUDE_DIR) -c $< -o $@

$(OBJS_DIR)/%.o : $(CLASS_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -I$(INCLUDES) -I$(LIBASYNC_INCLUDE_DIR) -I$(LIBCONTAINER_INCLUDE_DIR) -c $< -o $@

$(NAME) : $(OBJS) $(LIBASYNC_DEST)
	$(CXX) $(CXXFLAGS) $^ -I$(INCLUDES) -I$(LIBASYNC_INCLUDE_DIR) -I$(LIBCONTAINER_INCLUDE_DIR) -o $@

clean : libasync_clean libcontainer_clean
	$(RM) -r $(OBJS) $(DEPS) $(OBJS_DIR)

fclean : clean
	$(RM) $(NAME)

re:
	$(MAKE) fclean
	$(MAKE) all

-include $(DEPS)