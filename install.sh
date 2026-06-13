#!/usr/bin/env bash
set -euo pipefail

REPO="TurkFork/tinyshell.1"
BRANCH="main"
DEST=""

usage() {
    cat <<EOF
Usage: ${0##*/} [options]

Install TinyShell from GitHub.

Options:
  -d, --dest DIR     Install to DIR (default: /usr/local/bin or ~/.local/bin)
  -b, --branch BRANCH  Branch to fetch from (default: main)
  -h, --help         Show this help
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -d|--dest)    DEST="$2"; shift 2 ;;
        -b|--branch)  BRANCH="$2"; shift 2 ;;
        -h|--help)    usage ;;
        *) echo "unknown option: $1"; usage ;;
    esac
done

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

# --- choose destination ---
if [ -n "$DEST" ]; then
    dest="$DEST"
elif [ -w "/usr/local/bin" ]; then
    dest="/usr/local/bin"
else
    dest="$HOME/.local/bin"
fi
mkdir -p "$dest"

# --- download binary ---
url="https://raw.githubusercontent.com/$REPO/$BRANCH/builds/$binary"
echo "  downloading $binary ..."
if ! curl -sL --fail "$url" -o "$dest/tinyshell"; then
    echo "error: binary not found at $url"
    echo ""
    echo "  Builds are available for:"
    echo "    darwin-amd64, darwin-arm64"
    echo "    linux-amd64, linux-arm64, linux-armv7"
    exit 1
fi
chmod 755 "$dest/tinyshell"

# --- backup existing config ---
config="$HOME/.tinyshellrc"
if [ -f "$config" ]; then
    backup="${config}.bak"
    if [ ! -f "$backup" ]; then
        cp "$config" "$backup"
        echo "  backed up existing config to $backup"
    fi
fi

# --- download example config if none exists ---
if [ ! -f "$config" ]; then
    config_url="https://raw.githubusercontent.com/$REPO/$BRANCH/examples/tinyshellrc.basic"
    if curl -sL --fail "$config_url" -o "$config" 2>/dev/null; then
        echo "  created example config at $config"
    fi
fi

# --- verify ---
if "$dest/tinyshell" -c 'tsh -v' &>/dev/null; then
    version="$("$dest/tinyshell" -c 'tsh -v' 2>/dev/null)"
    echo ""
    echo "  installed: $dest/tinyshell"
    echo "  version:   $version"
else
    echo "error: installed binary failed to run"
    exit 1
fi

# --- PATH warning ---
case ":$PATH:" in
    *":$dest:"*) ;;
    *)
        echo ""
        echo "  warning: $dest is not on your PATH."
        echo "  add this to your shell rc file:"
        echo "    export PATH=\"\$PATH:$dest\""
        ;;
esac

echo "  done."
