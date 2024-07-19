#!/bin/bash

mkdir -p bin

CC=clang

if [ ! -f bin/deps.o ]; then
  ${CC} -w -pipe -Ofast -Iexternal/gunslinger -c external/impl.c -o bin/deps.o
fi

${CC} -O1 tools/embed.c -o bin/embed.bin
./bin/embed.bin src/glsl/fs.glsl
./bin/embed.bin src/glsl/vs.glsl

${CC} -Ofast -c -Iexternal/gunslinger -Iexternal/fe -I. src/unity.c -o bin/unity.o
${CC} bin/*.o -o bin/gunware.bin -lm

