#!/bin/sh
set -eu

YES="${YES:-0}"
DEST="${DEST:-}"

while [ $# -gt 0 ]; do
    case "$1" in
        -y|--yes) YES=1; shift ;;
        -d|--dest) DEST="$2"; shift 2 ;;
        -h|--help) echo "Usage: install.sh [-y] [-d DIR]"; exit 0 ;;
        *) echo "unknown: $1"; exit 1 ;;
    esac
done

ARCH="$(uname -m)"
OS="$(uname -s)"

# --- choose install directory ---
if [ -z "$DEST" ]; then
    if [ -w "/usr/local/bin" ]; then DEST="/usr/local/bin"
    else DEST="${HOME}/.local/bin"
    fi
fi
mkdir -p "$DEST"

# --- find source directory (when running from a clone) ---
SRCDIR=""
if [ -f "Makefile" ] && [ -f "src/main.c" ]; then
    SRCDIR="."
elif [ -f "../Makefile" ] && [ -f "../src/main.c" ]; then
    SRCDIR=".."
fi

if [ -n "$SRCDIR" ]; then
    echo "  building from source ..."
    (cd "$SRCDIR" && make) 2>&1
    cp "$SRCDIR/tinyshell" "$DEST/tinyshell"
    chmod 755 "$DEST/tinyshell"
elif command -v curl >/dev/null 2>&1; then
    BINARY="tinyshell-${OS}-${ARCH}"
    URL="https://raw.githubusercontent.com/TurkFork/tinyshell.1/main/builds/${BINARY}"
    echo "  downloading ${BINARY} ..."
    if ! curl -sL --fail "${URL}" -o "${DEST}/tinyshell"; then
        echo "  error: no prebuilt binary for your system at:"
        echo "    ${URL}"
        echo ""
        echo "  try: git clone https://github.com/TurkFork/tinyshell.1.git && cd tinyshell.1 && make && sudo make install"
        exit 1
    fi
    chmod 755 "${DEST}/tinyshell"
else
    echo "  error: no source directory found and curl not available"
    exit 1
fi

SHELL_PATH="${DEST}/tinyshell"

# --- verify ---
if ! "$SHELL_PATH" -c 'tsh -v' >/dev/null 2>&1; then
    echo "  error: installed binary failed to run"
    exit 1
fi

echo "  installed: $SHELL_PATH"

# --- set as default shell ---
if [ "$YES" = 1 ] || [ -t 0 ]; then
    if [ "$YES" = 0 ]; then
        printf "  set tinyshell as your default shell? [y/N] "
        read -r answer
    else
        answer="y"
    fi
    case "$answer" in
        y|Y|yes|YES)
            if grep -Fxq "$SHELL_PATH" /etc/shells 2>/dev/null; then
                true
            else
                echo "  adding $SHELL_PATH to /etc/shells ..."
                if [ -w /etc/shells ]; then
                    echo "$SHELL_PATH" >> /etc/shells
                else
                    echo "$SHELL_PATH" | sudo tee -a /etc/shells >/dev/null
                fi
            fi
            echo "  running chsh ..."
            if chsh -s "$SHELL_PATH"; then
                echo "  default shell changed to tinyshell (log out and back in)"
            else
                echo "  warning: chsh failed (wrong password?)"
            fi
            ;;
    esac
fi
