#!/usr/bin/env sh

set -euo pipefail

flags=""
flags+=" -lm"
flags+=" -L raylib/lib"
flags+=" -I raylib/include"
flags+=" -l raylib"
flags+=" -g"

clang -o mcts.bin platform_raylib.c $flags
