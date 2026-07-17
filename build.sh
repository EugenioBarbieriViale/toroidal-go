#!/bin/bash

set -xe

# emcc src/main.c -o main.html \
#   -Os -Wall \
#   ./raylib/src/libraylib.a \
#   -I. -I./raylib/src/raylib.h \
#   -L. -L./raylib/src/libraylib.a \
#   -s USE_GLFW=3 \
#   -s ASYNCIFY \
#   --shell-file ./raylib/src/shell.html \
#   -DPLATFORM_WEB

emcc src/test.c -o test.html \
  -Os -Wall \
  ./raylib/src/libraylib.a \
  -I. -I./raylib/src/raylib.h \
  -L. -L./raylib/src/libraylib.a \
  -s USE_GLFW=3 \
  --shell-file ./raylib/src/shell.html \
  -DPLATFORM_WEB

emrun test.html

# gcc src/main.c -o main \
#   -I raylib/src \
#   -L raylib/src \
#   -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
#   -g -O2 \
#   -Wall -Wextra \
#   -Wconversion \
#   -Wfloat-equal \
#   -Wunreachable-code \
#   -Wno-free-nonheap-object \
#   -Wdouble-promotion \
#   -Wformat \
#   -Wpedantic \
#   -fsanitize={address,undefined}
#
# ./main

# gcc src/rules.c -o rules \
#   -g -O2 \
#   -Wall -Wextra \
#   -Wconversion \
#   -Wfloat-equal \
#   -Wunreachable-code \
#   -Wno-free-nonheap-object \
#   -Wdouble-promotion \
#   -Wformat \
#   -Wpedantic \
#   -fsanitize={address,undefined}
#
# ./rules

exit
