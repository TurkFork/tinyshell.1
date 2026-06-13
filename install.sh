#!/bin/sh
set -eu

REPO="TurkFork/tinyshell.1"
BRANCH="${BRANCH:-main}"
DEST="${DEST:-}"

while [ $# -gt 0 ]; do
    case "$1" in
        -d) DEST="$2"; shift 2 ;;
        -b) BRANCH="$2"; shift 2 ;;
        -h) echo "Usage: install.sh [-d DIR] [-b BRANCH]"; exit 0 ;;
        *) echo "unknown: $1"; exit 1 ;;
    esac
done

case "$(uname -m)" in
    x86_64|amd64)  ARCH="amd64" ;;
    aarch64|arm64) ARCH="arm64" ;;
    armv7l|armv7)  ARCH="armv7" ;;
    *) echo "unsupported arch: $(uname -m)"; exit 1 ;;
esac

case "$(uname -s)" in
    Darwin) OS="darwin" ;;
    Linux)  OS="linux"  ;;
    *) echo "unsupported OS: $(uname -s)"; exit 1 ;;
esac

BINARY="tinyshell-${OS}-${ARCH}"

if [ -z "$DEST" ]; then
    if [ -w "/usr/local/bin" ]; then DEST="/usr/local/bin"
    else DEST="${HOME}/.local/bin"
    fi
fi
mkdir -p "$DEST"

URL="https://raw.githubusercontent.com/${REPO}/${BRANCH}/builds/${BINARY}"
echo "downloading ${BINARY} ..."
curl -sL --fail "${URL}" -o "${DEST}/tinyshell" || {
    echo "error: not found at ${URL}"
    exit 1
}
chmod 755 "${DEST}/tinyshell"

if "${DEST}/tinyshell" -c 'tsh -v' >/dev/null 2>&1; then
    echo "installed: ${DEST}/tinyshell"
else
    echo "error: binary failed to run"
    exit 1
fi
