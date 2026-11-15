#!/bin/sh
# Render each game screen to a PNG (headless review). Usage: shots.sh <outdir> name:"args" ...
OUT="$(mkdir -p "$1" && cd "$1" && pwd)"; shift
# UE 5.8: $UE if set, else the first install found (each team PC has it in a different place)
if [ -z "$UE" ]; then
  for c in "/c/Program Files/Epic Games/UE_5.8" "$HOME/Epic Games/UE_5.8" "/c/Users/ubart/Epic Games/UE_5.8"; do
    [ -f "$c/Engine/Binaries/Win64/UnrealEditor.exe" ] && UE="$c/Engine/Binaries/Win64/UnrealEditor.exe" && break
  done
fi
mkdir -p "$OUT"
for spec in "$@"; do
  name="${spec%%:*}"; args="${spec#*:}"
  rm -f "$OUT/$name.png"
  "$UE" "$(cygpath -m "$(cd "$(dirname "$0")/.." && pwd)")/Infinisat.uproject" -game -windowed -ResX=1600 -ResY=900 -nosplash -unattended -RenderOffscreen $args -ISShot="$(cygpath -m "$OUT")/$name.png" >/dev/null 2>&1
  [ -f "$OUT/$name.png" ] && echo "ok $name" || echo "MISSING $name"
done
