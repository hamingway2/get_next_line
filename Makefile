NAME = get_next_line.a
CC = cc
CFLAGS = -Wall -Wextra -Werror

HEADER = get_next_line.a
SRCS = get_next_line.c get_next_line_utils.c
OBJECTS = $(SRCS:.c=.o)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJECTS)
	@ar rcs $(NAME) $(OBJECTS)

all: $(NAME)
clean:
	@rm -f $(OBJECTS)
fclean: clean
	@rm -f $(NAME)
re: fclean all

.PHONY: all clean fclean re