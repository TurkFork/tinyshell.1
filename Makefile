CC=gcc
CFLAGS=-Wall -Wextra -Iinclude -g

SRC=src/main.c \
    src/parser.c \
    src/executor.c \
    src/prompt.c \
    src/builtins.c \
    src/color.c \
    src/input.c

OUT=tinyshell

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

install: all
	cp $(OUT) $(DESTDIR)/usr/local/bin/$(OUT)
	chmod 755 $(DESTDIR)/usr/local/bin/$(OUT)

clean:
	rm -f $(OUT)