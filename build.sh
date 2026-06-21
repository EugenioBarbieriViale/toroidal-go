#!/bin/bash

set -xe
gcc src/main.c -o main -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./main
rm main

exit
