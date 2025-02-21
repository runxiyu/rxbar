.PHONY: install

CFLAGS += -D_GNU_SOURCE -Wall -Wextra -pedantic -std=c99

rxbar: main.c
	$(CC) $(CFLAGS) -o rxbar main.c -lcjson


install: rxbar
	install -m 755 rxbar ${HOME}/.local/bin/rxbar
