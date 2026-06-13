#!/usr/bin/env bash
set -euo pipefail

SRC="src/main.c src/parser.c src/executor.c src/prompt.c src/builtins.c src/color.c src/input.c"
CFLAGS="-Wall -Wextra -Iinclude"
OUTDIR="builds"
JOBS=0

usage() {
    echo "Usage: $0 [-j N]"
    echo "  -j N   parallel builds (default: serial)"
    exit 1
}

while getopts "j:" opt; do
    case "$opt" in
        j) JOBS="$OPTARG" ;;
        *) usage ;;
    esac
done

build() {
    local cc="$1" cflags="$2" out="$3"
    echo "  -> $out"
    $cc $CFLAGS $cflags $SRC -o "$OUTDIR/$out"
}

echo "Building tinyshell for all targets..."

# --- Darwin / macOS ---
if command -v gcc &>/dev/null; then
    ( build "gcc" "-arch x86_64 -O2" "tinyshell-darwin-amd64" ) &
    ( build "gcc" "-arch arm64 -O2"  "tinyshell-darwin-arm64" ) &
fi

# --- Linux (musl — static, runs on any Linux) ---
# Install: brew install FiloSottile/musl-cross/musl-cross
for tuple in x86_64-linux-musl aarch64-linux-musl arm-linux-musleabihf; do
    cc="${tuple}-gcc"
    case "$tuple" in
        x86_64-linux-musl)  out="tinyshell-linux-amd64"  ;;
        aarch64-linux-musl) out="tinyshell-linux-arm64"  ;;
        arm-linux-musleabihf) out="tinyshell-linux-armv7" ;;
    esac
    if command -v "$cc" &>/dev/null; then
        ( build "$cc" "-O2 -static" "$out" ) &
    fi
done

wait

echo "Done. Built targets:"
ls -1 "$OUTDIR"/tinyshell-* 2>/dev/null | sed 's/^/  /' || echo "  (no binaries produced — install cross-compilers)"
