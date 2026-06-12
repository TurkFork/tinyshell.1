# TinyShell

A small Unix-like shell written in C.

## Features

- External command execution via `execvp`
- Built-in commands: `cd`, `pwd`, `echo`, `clear`, `exit`, `help`
- **Pipes**: `cmd1 | cmd2 | cmd3`
- **Redirection**: `>`, `>>`, `<`
- **Background**: `cmd &`
- **Sequential**: `cmd1 ; cmd2`
- **Quoting**: double (`"..."`) and single (`'...'`) quotes
- **Variable expansion**: `$VAR` and `${VAR}`
- **Tilde expansion**: `~` → `$HOME`
- **`cd -`**: return to previous directory
- **Exit code display**: shows `[N]` in prompt when non-zero

## Build

```bash
make
./tinyshell
```

## Structure

```
├── src/
│   ├── main.c        — read-eval loop
│   ├── parser.c      — line → command structures
│   ├── executor.c    — pipelines, forks, pipes, redirects
│   ├── prompt.c      — prompt with user@host ~/path
│   └── builtins.c    — cd, pwd, echo, clear, exit, help
├── include/
│   ├── command.h     — command/pipeline/line types
│   ├── parser.h
│   ├── executor.h
│   ├── builtins.h
│   └── prompt.h
├── Makefile
└── README.md
```

## How it works

1. Print prompt (with exit code if non-zero)
2. Read input
3. Parse into pipelines (split on `;`, then `|`, then tokens)
4. Execute each pipeline with pipes, redirects, and background support
5. Repeat
