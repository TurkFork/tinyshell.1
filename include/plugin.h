#ifndef TINYSHELL_PLUGIN_H
#define TINYSHELL_PLUGIN_H

#include <stdio.h>

/* Plugin command handler — receives NULL-terminated args array */
typedef int (*tsh_cmd_fn)(char **args);

/* A single command exported by a plugin */
typedef struct {
    const char *name;
    const char *description;
    tsh_cmd_fn fn;
} tsh_cmd_t;

/* Plugin descriptor — every plugin must export a `tsh_plugin_t tinyshell_plugin` symbol */
typedef struct {
    const char *name;
    const char *version;
    int ncmds;
    const tsh_cmd_t *cmds;
} tsh_plugin_t;

/* Plugin manager — called by the shell */
void init_plugins(void);
tsh_cmd_fn find_plugin_cmd(const char *name);
int list_plugins(FILE *out);
int load_plugin_cmd(const char *path);

#endif
