#!/bin/sh
# Phase 2 look comparison: every look (0 current, 1 golden, 2 morning, 3 overcast) on the same fixed frames.
# Usage: look_shots.sh <outdir> [extra args]
OUT="$1"; shift
B="-ISGraphics=2 -ISWorld -ISNoHUD -ISShotDelay=10 $*"
set -- "garage_in:-ISCam=-120,120,170,-7,116,70" "garage_out:-ISCam=-380,-420,150,-4,90,60" "yard:-ISCam=1400,2600,250,-6,235,70"
for L in 0 1 2 3; do
  case $L in 0) N=0_current;; 1) N=A_golden;; 2) N=B_morning;; 3) N=C_overcast;; esac
  for F in "$@"; do
    sh "$(dirname "$0")/shots.sh" "$OUT" "${F%%:*}_$N:$B ${F#*:} -ISLook=$L"
  done
done
