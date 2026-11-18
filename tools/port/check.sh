#!/bin/sh
# Compile game sources with the host flags of the root CMake build.
#   tools/port/check.sh                 game/native sources and standalone headers
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
# The ILP32 pass uses i686 MinGW headers to check native 32-bit layouts too.
# RR4_CHECK_ILP32=0 skips it; =1 requires it. RR4_MINGW32_SYSROOT overrides
# the shared MinGW sysroot when the two header sets live separately.
#
# With no arguments it also runs the layout sweep (sweep_layout.py) when the
# decompilation tree is there: RR4_DECOMP (default ../ridge-racer-type-4-decomp
# next to this repository) and a Python with the pinned libclang binding
# (tools/port/requirements-sweep.txt; RR4_SWEEP_PYTHON, default python3).
# RR4_CHECK_SWEEP=0 skips it, =1 makes a missing tree or binding fail.
#
# All passes need Clang (-Wno-everything, -ferror-limit, --target): CC
# defaults to clang, and any other compiler is refused.
cd "$(dirname "$0")/../.." || exit 1
CC=${CC:-clang}
if ! $CC --version 2> /dev/null | grep -qi clang; then
  echo "check.sh: CC=$CC is not Clang; the check uses Clang-only flags (set CC=clang)" >&2
  exit 2
fi
CFLAGS="-x c -std=c11 -c -o /dev/null -D__psyz -Iinclude -isystem third_party/psyz/psyz/include
 -fno-strict-aliasing -fwrapv -Wno-everything -Wint-conversion -Wpointer-to-int-cast
 -Wint-to-pointer-cast -Wimplicit-function-declaration -Wincompatible-pointer-types
 -Wstrict-prototypes -Werror -ferror-limit=0"
# Implicit 64-to-32-bit truncations and printf formats differ only under LLP64
# for long; elsewhere they are the same on every 64-bit host.
LLP64_FLAGS="-Wshorten-64-to-32 -Wformat"
full=0
[ $# -eq 0 ] && full=1 && set -- src/main src/overlays src/native include
LIST=$(for a in "$@"; do if [ -d "$a" ]; then find "$a" \( -name '*.c' -o -name '*.h' \); else echo "$a"; fi; done | sort)
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

# Discover the shared headers once. Probe the CRT as well as the target:
# compiling an empty file does not detect a missing or wrong sysroot.
sysroot=${RR4_MINGW_SYSROOT:-}
if [ -z "$sysroot" ] && command -v x86_64-w64-mingw32-gcc > /dev/null 2>&1; then
  sysroot=$(x86_64-w64-mingw32-gcc -print-sysroot 2> /dev/null)
  [ -n "$sysroot" ] || sysroot=$(dirname "$(dirname "$(command -v x86_64-w64-mingw32-gcc)")")
fi
has_target() { # triple, sysroot, pointer bytes
  [ -n "$2" ] || return 1
  $CC --target="$1" --sysroot="$2" -std=c11 -fsyntax-only -x c - > /dev/null 2>&1 <<EOF
#include <stdint.h>
#include <stdio.h>
_Static_assert(sizeof(void *) == $3, "target pointer width");
_Static_assert(sizeof(long) == 4, "MinGW long width");
EOF
}
if [ "${RR4_CHECK_LLP64:-auto}" != 0 ]; then
  if has_target x86_64-w64-mingw32 "$sysroot" 8; then
    run_pass LLP64 --target=x86_64-w64-mingw32 --sysroot="$sysroot" $LLP64_FLAGS || status=1
  elif [ "${RR4_CHECK_LLP64:-auto}" = 1 ]; then
    echo "check (LLP64): no Clang MinGW-w64 headers (install mingw-w64 or set RR4_MINGW_SYSROOT)"
    status=1
  else
    echo "check (LLP64): skipped, no Clang MinGW-w64 headers"
  fi
fi
if [ "${RR4_CHECK_ILP32:-auto}" != 0 ]; then
  sysroot32=${RR4_MINGW32_SYSROOT:-${RR4_MINGW_SYSROOT:-}}
  if [ -z "$sysroot32" ] && command -v i686-w64-mingw32-gcc > /dev/null 2>&1; then
    sysroot32=$(i686-w64-mingw32-gcc -print-sysroot 2> /dev/null)
    [ -n "$sysroot32" ] || sysroot32=$(dirname "$(dirname "$(command -v i686-w64-mingw32-gcc)")")
  fi
  sysroot32=${sysroot32:-$sysroot}
  if has_target i686-w64-mingw32 "$sysroot32" 4; then
    run_pass ILP32 --target=i686-w64-mingw32 --sysroot="$sysroot32" || status=1
  elif [ "${RR4_CHECK_ILP32:-auto}" = 1 ]; then
    echo "check (ILP32): no Clang i686 MinGW headers (install mingw-w64 or set RR4_MINGW32_SYSROOT)"
    status=1
  else
    echo "check (ILP32): skipped, no Clang i686 MinGW headers"
  fi
fi
if [ "$full" = 1 ] && [ "${RR4_CHECK_SWEEP:-auto}" != 0 ]; then
  decomp=${RR4_DECOMP:-../ridge-racer-type-4-decomp}
  py=${RR4_SWEEP_PYTHON:-python3}
  if [ -f "$decomp/configs/PAL/sym.main.txt" ] && "$py" -c 'import clang.cindex' > /dev/null 2>&1; then
    echo "check (layout sweep): $decomp"
    "$py" tools/port/sweep_layout.py "$decomp" || status=1
  elif [ "${RR4_CHECK_SWEEP:-auto}" = 1 ]; then
    echo "check (layout sweep): needs RR4_DECOMP and $py with tools/port/requirements-sweep.txt"
    status=1
  else
    echo "check (layout sweep): skipped, no decompilation tree or libclang binding"
  fi
fi
exit "$status"
