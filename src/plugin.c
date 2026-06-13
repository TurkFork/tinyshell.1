#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <limits.h>

#include "../include/plugin.h"

#define MAX_PLUGINS 64
#define MAX_PLUGIN_CMDS 256

typedef struct {
    void *handle;
    char name[64];
    char version[16];
    int ncmds;
} loaded_plugin_t;

typedef struct {
    char name[64];
    char desc[128];
    tsh_cmd_fn fn;
    char plugin[64];
} plugin_cmd_entry_t;

static loaded_plugin_t plugins[MAX_PLUGINS];
static int nplugins = 0;
static plugin_cmd_entry_t plugin_cmds[MAX_PLUGIN_CMDS];
static int nplugin_cmds = 0;

static int load_plugin(const char *path)
{
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle)
        return -1;

    tsh_plugin_t *p = dlsym(handle, "tinyshell_plugin");
    if (!p)
    {
        dlclose(handle);
        return -1;
    }

    if (nplugins >= MAX_PLUGINS)
    {
        dlclose(handle);
        return -1;
    }

    strncpy(plugins[nplugins].name, p->name, sizeof(plugins[nplugins].name) - 1);
    strncpy(plugins[nplugins].version, p->version, sizeof(plugins[nplugins].version) - 1);
    plugins[nplugins].handle = handle;
    plugins[nplugins].ncmds = p->ncmds;

    for (int i = 0; i < p->ncmds && nplugin_cmds < MAX_PLUGIN_CMDS; i++)
    {
        strncpy(plugin_cmds[nplugin_cmds].name, p->cmds[i].name,
                sizeof(plugin_cmds[nplugin_cmds].name) - 1);
        strncpy(plugin_cmds[nplugin_cmds].desc, p->cmds[i].description,
                sizeof(plugin_cmds[nplugin_cmds].desc) - 1);
        plugin_cmds[nplugin_cmds].fn = p->cmds[i].fn;
        strncpy(plugin_cmds[nplugin_cmds].plugin, p->name,
                sizeof(plugin_cmds[nplugin_cmds].plugin) - 1);
        nplugin_cmds++;
    }

    nplugins++;
    return 0;
}

void init_plugins(void)
{
    const char *home = getenv("HOME");
    if (!home) return;

    char dir[PATH_MAX];
    snprintf(dir, sizeof(dir), "%s/.tinyshell/plugins", home);

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *e;
    while ((e = readdir(d)))
    {
        if (e->d_name[0] == '.') continue;

        const char *dot = strrchr(e->d_name, '.');
        if (!dot) continue;

        if (strcmp(dot, ".so") != 0
#ifdef __APPLE__
            && strcmp(dot, ".dylib") != 0
#endif
        ) continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);

        if (load_plugin(path) == 0)
            printf("  plugin: loaded %s (%s)\n", plugins[nplugins - 1].name, e->d_name);
    }
    closedir(d);
}

tsh_cmd_fn find_plugin_cmd(const char *name)
{
    for (int i = 0; i < nplugin_cmds; i++)
        if (strcmp(plugin_cmds[i].name, name) == 0)
            return plugin_cmds[i].fn;
    return NULL;
}

int list_plugins(FILE *out)
{
    if (nplugins == 0)
    {
        fprintf(out, "  no plugins loaded\n");
        return 0;
    }
    for (int i = 0; i < nplugins; i++)
    {
        fprintf(out, "  %s (%s) — %d command%s\n",
                plugins[i].name, plugins[i].version,
                plugins[i].ncmds, plugins[i].ncmds == 1 ? "" : "s");
        for (int j = 0; j < nplugin_cmds; j++)
        {
            if (strcmp(plugin_cmds[j].plugin, plugins[i].name) == 0)
                fprintf(out, "    %s — %s\n", plugin_cmds[j].name, plugin_cmds[j].desc);
        }
    }
    return nplugins;
}

int load_plugin_cmd(const char *path)
{
    return load_plugin(path);
}
