# TinyShell

A small Unix-like shell written in C (macOS/Linux).

## Quick Install

```bash
curl -sL https://raw.githubusercontent.com/TurkFork/tinyshell.1/main/install.sh | bash
```

Or build from source:

```bash
make && ./tinyshell
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
- **`tsh` subcommand**:
  - `tsh -v` / `--version` — show version
  - `tsh update` / `check` — check GitHub releases for updates (via curl)
  - `tsh history` — show last 100 commands
  - `tsh help` — usage
- **SIGINT handling**: Ctrl+C interrupts current command without exiting the shell

### Interactive mode (when stdin is a TTY)

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

| Platform | Binary |
|---|---|
| macOS (Apple Silicon) | `builds/tinyshell-darwin-arm64` |
| macOS (Intel) | `builds/tinyshell-darwin-amd64` |
| Linux (amd64) | `cross_build.sh` with `x86_64-linux-gnu-gcc` |
| Linux (arm64) | `cross_build.sh` with `aarch64-linux-gnu-gcc` |
| Linux (armv7) | `cross_build.sh` with `arm-linux-gnueabihf-gcc` |
| Linux (riscv64) | `cross_build.sh` with `riscv64-linux-gnu-gcc` |
| FreeBSD (amd64) | `cross_build.sh` with `x86_64-pc-freebsd13-gcc` |

Run `./cross_build.sh` to build for all targets with available cross-compilers.

## Project structure

```
├── builds/             — prebuilt binaries
├── examples/
│   └── tinyshellrc.basic
├── include/
│   ├── command.h       — command/pipeline/line types
│   ├── parser.h
│   ├── executor.h
│   ├── builtins.h
│   ├── prompt.h
│   ├── input.h
│   ├── color.h
│   └── version.h
├── src/
│   ├── main.c          — read-eval loop, SIGINT, config
│   ├── parser.c        — line → command structures
│   ├── executor.c      — pipelines, forks, pipes, redirects
│   ├── input.c         — raw terminal, line editing, history, tab complete
│   ├── prompt.c        — prompt with user@host ~/path
│   ├── builtins.c      — cd, pwd, echo, clear, exit, export, tsh, help
│   └── color.c         — ANSI color helpers
├── cross_build.sh
├── install.sh
├── Makefile
└── README.md
```
