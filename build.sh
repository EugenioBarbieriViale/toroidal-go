#!/bin/bash

set -xe
gcc src/main.c -o main \
  -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
  -g -O2 \
  -Wall -Wextra \
  -Wunused \
  -Wconversion \
  -Wfloat-equal \
  -Wunreachable-code \
  -Wuninitialized \
  -Wno-free-nonheap-object \
  -fsanitize={address,undefined}

./main
# rm main

exit
