CFLAGS = -g -Os -Wall -Wextra -Wconversion -Wfloat-equal -Wunreachable-code -Wno-free-nonheap-object -Wdouble-promotion -Wformat -Wpedantic -fsanitize={address,undefined}

client:
	gcc -o client client.c httplib.c $(CFLAGS)

server:
	gcc -o server server.c httplib.c ../src/rules.c ../src/stack.c $(CFLAGS)

clean:
	rm client
	rm server

.PHONY: client server
