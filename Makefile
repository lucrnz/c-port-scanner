CC = cc
CFLAGS = -pthread -fsanitize=address -g -Wall -Werror

pscanner: pscanner.c
	$(CC) $(CFLAGS) -o pscanner pscanner.c

.PHONY: clean

clean:
	rm -f pscanner
