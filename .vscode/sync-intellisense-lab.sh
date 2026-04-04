#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
LAB_MK="$ROOT_DIR/conf/lab.mk"
OUT_FILE="$ROOT_DIR/.vscode/lab_intellisense_defines.h"

lab=""
if [[ -f "$LAB_MK" ]]; then
  # Read the first non-comment LAB assignment, accepting optional whitespace.
  line="$(grep -E '^[[:space:]]*LAB[[:space:]]*=' "$LAB_MK" | head -n 1 || true)"
  if [[ -n "$line" ]]; then
    lab="${line#*=}"
    lab="${lab%%#*}"
    lab="${lab//[[:space:]]/}"
  fi
fi

{
  echo "/* Auto-generated from conf/lab.mk. Do not edit manually. */"
  echo "#ifndef XV6_LAB_INTELLISENSE_DEFINES_H"
  echo "#define XV6_LAB_INTELLISENSE_DEFINES_H"
  if [[ -n "$lab" ]]; then
    lab_upper="$(echo "$lab" | tr '[:lower:]' '[:upper:]')"
    echo "#define LAB_${lab_upper} 1"
    echo "#define SOL_${lab_upper} 1"
  fi
  echo "#endif"
} > "$OUT_FILE"
