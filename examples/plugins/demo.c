#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../../include/plugin.h"

static int cmd_hello(char **args)
{
    printf("hello from demo plugin! (%s)\n", args[0]);
    if (args[1])
        printf("  arg: %s\n", args[1]);
    return 0;
}

static int cmd_time(char **args)
{
    (void)args;
    time_t t = time(NULL);
    printf("%s", ctime(&t));
    return 0;
}

static int cmd_echo_cmd(char **args)
{
    for (int i = 1; args[i]; i++)
        printf("%s%s", i > 1 ? " " : "", args[i]);
    printf("\n");
    return 0;
}

static const tsh_cmd_t cmds[] = {
    {"hello",    "Print a greeting",                     cmd_hello},
    {"time",     "Show current time",                    cmd_time},
    {"echocmd",  "Echo arguments (plugin version)",      cmd_echo_cmd},
};

tsh_plugin_t tinyshell_plugin = {
    .name = "demo",
    .version = "1.0",
    .ncmds = 3,
    .cmds = cmds,
};
