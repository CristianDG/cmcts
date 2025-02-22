#!/usr/bin/env bash

set -euo pipefail

# TODO: colocar um alerta se rpi e windows são 1?
ON_RPI=0
ON_WINDOWS=0

if [[ $ON_RPI == 1 ]]; then
  $ON_WINDOWS=0
fi

flags=""
flags+=" -std=c11"
flags+=" -L ./raylib/lib"
flags+=" -I ./raylib/include"
flags+=" -g"

if [[ $ON_WINDOWS == 1 ]]; then
  flags+=" -lmsvcrt -lOpenGL32 -lGdi32 -lWinMM -lkernel32 -lshell32 -lUser32 -Xlinker /NODEFAULTLIB:libcmt"
else  
  flags+=" -lm -ldl -pthread"
fi

if [[ $ON_RPI == 1 ]]; then
  flags+=" -l raylib_rpi"
else  
  flags+=" -l raylib"
fi


clang -o mcts.exe platform_terminal.c $flags
