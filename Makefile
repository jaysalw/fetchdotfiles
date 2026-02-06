CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lncurses
SRC = src/main.c src/parser.c src/file_ops.c src/utils.c src/diff_viewer.c
TARGET = fdf

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
