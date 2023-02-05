CC = cc
CFLAGS = -fsanitize=address -g -Wall -Werror

pscanner: main.c
	$(CC) $(CFLAGS) -o pscanner main.c

.PHONY: clean

clean:
	rm -f pscanner
