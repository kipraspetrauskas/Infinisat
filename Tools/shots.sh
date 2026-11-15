#!/bin/sh
# Render each game screen to a PNG (headless review). Usage: shots.sh <outdir> name:"args" ...
OUT="$(mkdir -p "$1" && cd "$1" && pwd)"; shift
UE="${UE:-/c/Users/ubart/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe}"
mkdir -p "$OUT"
for spec in "$@"; do
  name="${spec%%:*}"; args="${spec#*:}"
  rm -f "$OUT/$name.png"
  "$UE" "$(cygpath -m "$(cd "$(dirname "$0")/.." && pwd)")/Infinisat.uproject" -game -windowed -ResX=1600 -ResY=900 -nosplash -unattended -RenderOffscreen $args -ISShot="$(cygpath -m "$OUT")/$name.png" >/dev/null 2>&1
  [ -f "$OUT/$name.png" ] && echo "ok $name" || echo "MISSING $name"
done
