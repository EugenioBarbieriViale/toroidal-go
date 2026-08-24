CC = gcc
EMCC = emcc

SRC_FILES = src/client/rules.c src/client/stack.c src/client/net.c src/client/httplib.c src/client/gxf.c

CFLAGS = -g -Os -I include 
DBG_CFLAGS = $(CFLAGS) $(SRC_FILES) -Wall -Wextra -Wconversion -Wfloat-equal -Wunreachable-code -Wno-free-nonheap-object -Wdouble-promotion -Wformat -Wpedantic -fsanitize={address,undefined}
WEB_CFLAGS = $(SRC_FILES) -Os -s USE_GLFW=3 -s TOTAL_STACK=64MB -s INITIAL_MEMORY=128MB -s ASSERTIONS --preload-file web/assets -DPLATFORM_WEB --shell-file web/shell.html

EMSDK_ENV = /home/eu/programming/tgo/external/emsdk/emsdk_env.sh

RAYLIB_SRC = /home/eu/programming/tgo/external/raylib/src
RAYLIB_WEB_LIB = $(RAYLIB_SRC)/libraylib.a
RAYLIB_WEB_CFLAGS = -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
RAYLIB_WEB_OBJS = rcore.o rshapes.o rtextures.o rtext.o rmodels.o raudio.o

native: src/client/main.c
	$(CC) -o client src/client/main.c \
		-lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
		$(CFLAGS) $(SRC_FILES)

web: src/client/main.c
	mkdir -p web/build
	$(EMCC) src/client/main.c -o web/build/main.html \
		./external/raylib/src/libraylib.a \
		-I. -I./external/raylib/src/raylib.h \
		-L. -L./external/raylib/src/libraylib.a \
		-I include \
		$(WEB_CFLAGS)

debug: src/client/main.c 
	$(CC) -o client src/client/main.c \
		-lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
		$(DBG_CFLAGS)

web_raylib:
	source $(EMSDK_ENV) && \
	cd $(RAYLIB_SRC) && \
	emcc -c rcore.c     $(RAYLIB_WEB_CFLAGS) && \
	emcc -c rshapes.c   $(RAYLIB_WEB_CFLAGS) && \
	emcc -c rtextures.c $(RAYLIB_WEB_CFLAGS) && \
	emcc -c rtext.c     $(RAYLIB_WEB_CFLAGS) && \
	emcc -c rmodels.c   $(RAYLIB_WEB_CFLAGS) && \
	emcc -c raudio.c    -Os -Wall -DPLATFORM_WEB && \
	emar rcs libraylib.a $(WEB_OBJS)

run:
	./client

run_web:
	cd web/build && python -m http.server 8080

clean:
	rm client
