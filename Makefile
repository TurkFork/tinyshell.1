CC=gcc
CFLAGS=-Wall -Wextra -Iinclude -g
LDFLAGS=-ldl

SRC=src/main.c \
    src/parser.c \
    src/executor.c \
    src/prompt.c \
    src/builtins.c \
    src/color.c \
    src/input.c \
    src/plugin.c

OUT=tinyshell

all:
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(OUT)

examples/plugins/demo.so: examples/plugins/demo.c
	mkdir -p examples/plugins
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@

plugins: all examples/plugins/demo.so

run: all
	./$(OUT)

install: all
	cp $(OUT) $(DESTDIR)/usr/local/bin/$(OUT)
	chmod 755 $(DESTDIR)/usr/local/bin/$(OUT)

clean:
	rm -f $(OUT)