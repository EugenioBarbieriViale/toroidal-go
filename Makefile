CC = gcc
EMCC = emcc
DBG_CFLAGS = -g -Wall -Wextra -Wconversion -Wfloat-equal -Wunreachable-code -Wno-free-nonheap-object -Wdouble-promotion -Wformat -Wpedantic -fsanitize={address,undefined}

native: src/main.c
	$(CC) src/main.c -o main \
		-I raylib/src \
		-L raylib/src \
		-lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
		-g -O2

web: src/main.c
	$(EMCC) src/main.c -o main.html \
		./external/raylib/src/libraylib.a \
		-I. -I./external/raylib/src/raylib.h \
		-L. -L./external/raylib/src/libraylib.a \
		-s USE_GLFW=3 \
		-s ALLOW_MEMORY_GROWTH=1 \
	 	-s TOTAL_STACK=64MB \
    -s INITIAL_MEMORY=128MB \
    -s ASSERTIONS \
		--preload-file assets \
		-DPLATFORM_WEB \
		-Os

debug: src/main.c
	$(CC) src/main.c -o main \
		-I raylib/src \
		-L raylib/src \
		-lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
		$(DBG_CFLAGS)

run:
	./main

run_web:
	python -m http.server 8080

clean:
	rm main
