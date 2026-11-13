#!/bin/sh
# Compile game sources with the host flags of the root CMake build.
#   tools/port/check.sh                 every file under src/main and src/overlays
#   tools/port/check.sh src/main/gfx    one directory
#   tools/port/check.sh a.c b.c         given files
# Prints each failing file with its errors and a summary line.
#
# A second pass compiles the same files for 64-bit Windows (LLP64: long is
# 32 bits, pointers 64) with Clang's MinGW target, so code that keeps a
# pointer or a 64-bit size in long / unsigned long fails here on macOS and
# Linux too. It needs the MinGW-w64 headers (x86_64-w64-mingw32-gcc on PATH,
# or RR4_MINGW_SYSROOT); without them it is skipped. RR4_CHECK_LLP64=0
# skips it, =1 makes a missing toolchain fail.
#
# Both passes need Clang (-Wno-everything, -ferror-limit, --target): CC
# defaults to clang, and any other compiler is refused.
cd "$(dirname "$0")/../.." || exit 1
CC=${CC:-clang}
if ! $CC --version 2> /dev/null | grep -qi clang; then
  echo "check.sh: CC=$CC is not Clang; the check uses Clang-only flags (set CC=clang)" >&2
  exit 2
fi
CFLAGS="-std=c11 -c -o /dev/null -D__psyz -Iinclude -isystem third_party/psyz/psyz/include
 -fno-strict-aliasing -fwrapv -Wno-everything -Wint-conversion -Wpointer-to-int-cast
 -Wint-to-pointer-cast -Wimplicit-function-declaration -Wincompatible-pointer-types
 -Werror -ferror-limit=0"
# Implicit 64-to-32-bit truncations and printf formats differ only under LLP64
# for long; elsewhere they are the same on every 64-bit host.
LLP64_FLAGS="-Wshorten-64-to-32 -Wformat"
[ $# -eq 0 ] && set -- src/main src/overlays
LIST=$(for a in "$@"; do if [ -d "$a" ]; then find "$a" -name '*.c'; else echo "$a"; fi; done | sort)
total=$(echo "$LIST" | wc -l | tr -d ' ')

# run_pass <name> <extra flags...>: compiles LIST, prints failures; fails if any.
run_pass() {
  name=$1
  shift
  TMP=$(mktemp -d)
  for f in $LIST; do
    ( $CC $CFLAGS "$@" "$f" > "$TMP/$(echo "$f" | tr / _).log" 2>&1 || : > "$TMP/$(echo "$f" | tr / _).failed" ) &
    while [ "$(jobs -r | wc -l)" -ge 8 ]; do sleep 0.05; done
  done
  wait
  failed=0
  for f in $LIST; do
    key=$(echo "$f" | tr / _)
    if [ -e "$TMP/$key.failed" ]; then
      failed=$((failed + 1))
      echo "=== $f${name:+ ($name)}"
      grep -E 'error:' "$TMP/$key.log"
    fi
  done
  rm -rf "$TMP"
  echo "check${name:+ ($name)}: $failed failing of $total files"
  [ "$failed" -eq 0 ]
}

status=0
run_pass "" || status=1

if [ "${RR4_CHECK_LLP64:-auto}" != 0 ]; then
  sysroot=${RR4_MINGW_SYSROOT:-}
  if [ -z "$sysroot" ] && command -v x86_64-w64-mingw32-gcc > /dev/null 2>&1; then
    sysroot=$(x86_64-w64-mingw32-gcc -print-sysroot 2> /dev/null)
    [ -n "$sysroot" ] || sysroot=$(dirname "$(dirname "$(command -v x86_64-w64-mingw32-gcc)")")
  fi
  if [ -n "$sysroot" ] && echo 'int x;' | $CC --target=x86_64-w64-mingw32 --sysroot="$sysroot" \
      -fsyntax-only -x c - > /dev/null 2>&1; then
    run_pass LLP64 --target=x86_64-w64-mingw32 --sysroot="$sysroot" $LLP64_FLAGS || status=1
  elif [ "${RR4_CHECK_LLP64:-auto}" = 1 ]; then
    echo "check (LLP64): no Clang MinGW-w64 target (install mingw-w64 or set RR4_MINGW_SYSROOT)"
    status=1
  else
    echo "check (LLP64): skipped, no Clang MinGW-w64 target (install mingw-w64)"
  fi
fi
exit "$status"
