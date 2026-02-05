CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRC = src/main.c src/parser.c src/file_ops.c src/utils.c
TARGET = fdf

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
