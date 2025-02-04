#!/usr/bin/env bash

set -euo pipefail

ON_WINDOWS=0

flags=""
flags+=" -std=c11"
flags+=" -L ./raylib/lib"
flags+=" -I ./raylib/include"
flags+=" -l raylib_rpi"
flags+=" -g"

if [[ $ON_WINDOWS == 1 ]]; then
  flags+=" -lmsvcrt -lOpenGL32 -lGdi32 -lWinMM -lkernel32 -lshell32 -lUser32 -Xlinker /NODEFAULTLIB:libcmt"
else  
  flags+=" -lm -ldl -pthread"
fi


clang -o mcts.exe platform_terminal.c $flags 
