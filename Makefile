CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRC = src/main.c src/parser.c src/file_ops.c src/utils.c
OBJ = $(SRC:.c=.o)
TARGET = fdf

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) $(TARGET)
