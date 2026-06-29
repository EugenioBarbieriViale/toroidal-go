#!/bin/bash

set -xe
gcc src/main.c -o main \
  -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
  -g -O2 \
  -Wall -Wextra \
  -Wconversion \
  -Wfloat-equal \
  -Wunreachable-code \
  -Wno-free-nonheap-object \
  -Wdouble-promotion \
  -Wformat \
  -Wpedantic \
  -fsanitize={address,undefined}

./main
# rm main

exit
