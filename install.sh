#!/usr/bin/env bash
set -euo pipefail

# --- detect target ---
arch=""
case "$(uname -m)" in
    x86_64|amd64)  arch="amd64" ;;
    aarch64|arm64) arch="arm64" ;;
    armv7l|armv7)  arch="armv7" ;;
    riscv64)       arch="riscv64" ;;
    *) echo "unsupported arch: $(uname -m)"; exit 1 ;;
esac

os=""
case "$(uname -s)" in
    Darwin) os="darwin" ;;
    Linux)  os="linux"  ;;
    FreeBSD) os="freebsd" ;;
    *) echo "unsupported OS: $(uname -s)"; exit 1 ;;
esac

binary="tinyshell-${os}-${arch}"
src_dir="$(cd "$(dirname "$0")" && pwd)"
src="$src_dir/builds/$binary"

if [ ! -f "$src" ]; then
    echo "error: no prebuilt binary for $os/$arch"
    echo "  (expected: $src)"
    echo ""
    echo "Try building from source:  make"
    exit 1
fi

# --- choose install directory ---
if [ -w "/usr/local/bin" ]; then
    dest="/usr/local/bin"
elif [ -w "$HOME/.local/bin" ]; then
    dest="$HOME/.local/bin"
else
    dest="$HOME/.local/bin"
    mkdir -p "$dest"
fi

cp "$src" "$dest/tinyshell"
chmod 755 "$dest/tinyshell"

echo "installed tinyshell to $dest/tinyshell"

# --- optional: copy example config ---
config_dest="$HOME/.tinyshellrc"
if [ ! -f "$config_dest" ] && [ -f "$src_dir/examples/tinyshellrc.basic" ]; then
    cp "$src_dir/examples/tinyshellrc.basic" "$config_dest"
    echo "created example config at $config_dest"
fi

# --- ensure dest is on PATH ---
case ":$PATH:" in
    *":$dest:"*) ;;
    *) echo "warning: $dest is not on your PATH. add it to your shell rc file:" ;;
esac

echo "done."
