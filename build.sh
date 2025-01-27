#!/usr/bin/env sh

set -euo pipefail

ON_WINDOWS=1
flags=""
flags+=" -L ./raylib/lib"
flags+=" -I ./raylib/include"
flags+=" -l raylib"
flags+=" -g"

if [[ $ON_WINDOWS == 1 ]]; then
  flags+=" -lmsvcrt -lOpenGL32 -lGdi32 -lWinMM -lkernel32 -lshell32 -lUser32 -Xlinker /NODEFAULTLIB:libcmt"
else  
  flags+=" -lm"
fi


clang -o mcts.exe platform_raylib.c $flags 
