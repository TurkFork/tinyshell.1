#!/usr/bin/env bash
set -euo pipefail

SRC="src/main.c src/parser.c src/executor.c src/prompt.c src/builtins.c src/color.c src/input.c"
CFLAGS="-Wall -Wextra -Iinclude"
OUTDIR="builds"

build() {
    local cc="$1"    cflags="$2"    out="$3"
    echo "  -> $out"
    $cc $CFLAGS $cflags $SRC -o "$OUTDIR/$out"
}

echo "Building tinyshell for all targets..."

# --- Darwin / macOS ---
if command -v gcc &>/dev/null; then
    build "gcc"         "-arch x86_64"  "tinyshell-darwin-amd64"
    build "gcc"         "-arch arm64"   "tinyshell-darwin-arm64"
fi

# --- Linux ---
if command -v x86_64-linux-gnu-gcc &>/dev/null; then
    build "x86_64-linux-gnu-gcc"     ""  "tinyshell-linux-amd64"
fi
if command -v aarch64-linux-gnu-gcc &>/dev/null; then
    build "aarch64-linux-gnu-gcc"    ""  "tinyshell-linux-arm64"
fi
if command -v arm-linux-gnueabihf-gcc &>/dev/null; then
    build "arm-linux-gnueabihf-gcc"  ""  "tinyshell-linux-armv7"
fi
if command -v riscv64-linux-gnu-gcc &>/dev/null; then
    build "riscv64-linux-gnu-gcc"    ""  "tinyshell-linux-riscv64"
fi

# --- FreeBSD ---
if command -v x86_64-pc-freebsd13-gcc &>/dev/null; then
    build "x86_64-pc-freebsd13-gcc"  ""  "tinyshell-freebsd-amd64"
fi

echo "Done. Built targets:"
ls -1 "$OUTDIR"/tinyshell-* 2>/dev/null | sed 's/^/  /' || echo "  (no binaries produced — install cross-compilers)"
