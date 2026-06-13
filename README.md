# TinyShell

A small Unix-like shell written in C (macOS/Linux).

## Install

```bash
# from the repo
./install.sh

# via curl (requires builds/ to be pushed):
curl -sL https://raw.githubusercontent.com/TurkFork/tinyshell.1/main/install.sh | bash

# build from source (always works):
make && sudo cp tinyshell /usr/local/bin/
```

## Usage

```
tinyshell            interactive shell
tinyshell -c <cmd>   run a command non-interactively
tsh help             built-in help
```

## Features

- **Builtins**: `cd` / `cd -`, `pwd`, `echo`, `clear`, `exit [n]`, `export`, `tsh`, `help`
- **Pipes**: `cmd1 | cmd2 | cmd3`
- **Redirection**: `>`, `>>`, `<` (with quoted filenames)
- **Background**: `cmd &`
- **Sequential**: `cmd1 ; cmd2`
- **Quoting**: double (`"..."`) and single (`'...'`) quotes — embedded quotes in tokens like `KEY="VALUE"` work correctly
- **Variable expansion**: `$VAR` and `${VAR}` in words and double-quoted strings
- **Exit code**: `$?` expands to last exit code; `[N]` shown in prompt when non-zero
- **Tilde expansion**: `~` → `$HOME`
- **Colors**: prompt in green/cyan, errors in red, help headers in bold (disable with `TINYSHELL_NO_COLOR=1`)
- **Config file**: `~/.tinyshellrc` sourced at startup (`#` comments, runs shell commands)
- **SIGINT handling**: Ctrl+C interrupts current command without exiting the shell

### `tsh` subcommand

| Command | Action |
|---|---|
| `tsh -v`, `--version` | Show version |
| `tsh update`, `check` | Check GitHub releases for updates |
| `tsh history` | Show last 100 commands |
| `tsh help` | Usage |

### Interactive mode (TTY only)

| Key | Action |
|---|---|
| **↑ / ↓** | Navigate history |
| **← / →** | Move cursor |
| **Home / End** | Jump to start/end |
| **Backspace / Delete** | Delete before/at cursor |
| **Tab** | Autocomplete commands (builtins + PATH) & files |
| **Ctrl+C** | Cancel current line |
| **Ctrl+D** | Exit (on empty line) |

History is persisted in `~/.tinyshell_history`.

## Prebuilt binaries

| Platform | Path |
|---|---|
| macOS Apple Silicon | `builds/tinyshell-darwin-arm64` |
| macOS Intel | `builds/tinyshell-darwin-amd64` |
| Linux x86_64 | `builds/tinyshell-linux-amd64` |
| Linux ARM64 | `builds/tinyshell-linux-arm64` |
| Linux ARMv7 | `builds/tinyshell-linux-armv7` |

All Linux binaries are statically linked against musl — run on any Linux with no dependencies.

Run `./cross_build.sh` to rebuild all targets (requires `brew install FiloSottile/musl-cross/musl-cross` for Linux targets).

## Project structure

```
├── builds/             — prebuilt binaries (5 targets)
├── examples/
│   └── tinyshellrc.basic
├── include/            — headers
├── src/                — source
│   ├── main.c          — read-eval loop, SIGINT, config, -c flag
│   ├── parser.c        — line → command structures
│   ├── executor.c      — pipelines, forks, pipes, redirects
│   ├── input.c         — raw terminal, line editing, history, tab complete
│   ├── builtins.c      — cd, pwd, echo, clear, exit, export, tsh, help
│   ├── prompt.c        — prompt builder
│   └── color.c         — ANSI color helpers
├── cross_build.sh      — cross-compile for all targets
├── install.sh          — detect OS/arch, install binary + config
├── Makefile
└── README.md
```
