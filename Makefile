CC = c++
CFLAGS += -g3 -Wall -Wextra -Werror -std=c++98

SRCS_DIR = srcs
SRCS = main.cpp \
		init.cpp \
		html_generator/html_error.cpp \
		html_generator/html_dir.cpp \
		url_encoding/url_decoder.cpp

CLASS_DIR = class
CLASS = ABackend.cpp \
		RequestPool.cpp \
		Date.cpp \
		FdStream.cpp \
		Headers.cpp \
		Response.cpp \
		Request.cpp \
		StaticBackend.cpp

OBJS_DIR = objs
OBJS = $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(SRCS)) $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(CLASS))

DEPS = $(patsubst %.cpp, $(OBJS_DIR)/%.d, $(SRCS)) $(patsubst %.cpp, $(OBJS_DIR)/%.d, $(CLASS))

DEPFLAGS = -MMD

INCLUDES = includes

NAME = webserv

.PHONY = $(NAME) all clean fclean re

all : $(NAME)

$(OBJS_DIR)/%.o : $(SRCS_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -I$(INCLUDES) -c $< -o $@

$(OBJS_DIR)/%.o : $(CLASS_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -I$(INCLUDES) -c $< -o $@

$(NAME) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -I$(INCLUDES) -o $(NAME)

clean :
	$(RM) -r $(OBJS) $(DEPS) $(OBJS_DIR)

fclean : clean
	$(RM) $(NAME)

re : fclean all

-include $(DEPS)
