CC = c++
CFLAGS += -g3 -Wall -Wextra -Werror -std=c++98

SRCS_DIR = srcs
SRCS = main.cpp \
		init.cpp \
		html_generator/html_error.cpp \
		html_generator/html_dir.cpp \
		string_overload/string_overload.cpp \
		url_encoding/file_extension.cpp \
		url_encoding/url_decoder.cpp \
		url_encoding/url_encoder.cpp

CLASS_DIR = class
CLASS = ABackend.cpp \
		BackendHolder.cpp \
		RequestPool.cpp \
		CGIBackend.cpp \
		Date.cpp \
		FdStream.cpp \
		Response.cpp \
		Request.cpp \
		Router.cpp \
		StaticBackend.cpp

OBJS_DIR = objs
OBJS = $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(SRCS)) $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(CLASS))

DEPS = $(patsubst %.cpp, $(OBJS_DIR)/%.d, $(SRCS)) $(patsubst %.cpp, $(OBJS_DIR)/%.d, $(CLASS))

DEPFLAGS = -MMD

INCLUDES = includes

NAME = webserv

LIBASYNC_DIR = libasync
LIBASYNC = $(LIBASYNC_DIR)/libasync.a
LIBASYNC_INC = $(LIBASYNC_DIR)/include

.PHONY = $(NAME) all clean fclean re $(LIBASYNC)

all : $(NAME)

$(LIBASYNC):
	$(MAKE) -C $(LIBASYNC_DIR)

$(OBJS_DIR)/%.o : $(SRCS_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -I$(INCLUDES) -I$(LIBASYNC_INC) -c $< -o $@

$(OBJS_DIR)/%.o : $(CLASS_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -I$(INCLUDES) -I$(LIBASYNC_INC) -c $< -o $@

$(NAME) : $(OBJS) $(LIBASYNC)
	$(CC) $(CFLAGS) $(OBJS) -I$(INCLUDES) -I$(LIBASYNC_INC) -o $(NAME)

clean :
	$(RM) -r $(OBJS) $(DEPS) $(OBJS_DIR)
	$(MAKE) -C $(LIBASYNC_DIR) clean

fclean : clean
	$(RM) $(NAME)
	$(MAKE) -C $(LIBASYNC_DIR) fclean

re : fclean all

-include $(DEPS)
