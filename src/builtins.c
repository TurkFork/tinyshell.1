#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "../include/color.h"

extern char **environ;

static char *prev_dir = NULL;

int builtin_cd(char **args)
{
    char *target;

    if (args[1] == NULL || strcmp(args[1], "~") == 0)
    {
        target = getenv("HOME");
        if (!target)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    }
    else if (strcmp(args[1], "-") == 0)
    {
        if (!prev_dir)
        {
            fprintf(stderr, "cd: no previous directory\n");
            return 1;
        }
        target = prev_dir;
        printf("%s\n", target);
    }
    else
    {
        target = args[1];
    }

    char old_cwd[PATH_MAX];
    int have_old = getcwd(old_cwd, sizeof(old_cwd)) != NULL;

    if (chdir(target) != 0)
    {
        perror("cd");
        return 1;
    }

    if (have_old)
    {
        free(prev_dir);
        prev_dir = strdup(old_cwd);
    }

    return 0;
}

int builtin_pwd(char **args)
{
    (void)args;
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd);
    }
    else
    {
        perror("pwd");
        return 1;
    }

    return 0;
}

int builtin_help(char **args)
{
    (void)args;
    const char *h = use_color() ? C_BOLD : "";
    const char *r = use_color() ? C_RESET : "";

    printf("%sTinyShell%s — built-in commands:\n", h, r);
    printf("  %scd%s [dir]      Change directory (~, -, no arg = home)\n",
           use_color() ? C_GREEN : "", r);
    printf("  %spwd%s           Print working directory\n",
           use_color() ? C_GREEN : "", r);
    printf("  %secho%s <text>   Print text\n",
           use_color() ? C_GREEN : "", r);
    printf("  %sclear%s         Clear screen\n",
           use_color() ? C_GREEN : "", r);
    printf("  %sexit%s [n]      Exit shell (default: 0)\n",
           use_color() ? C_GREEN : "", r);
    printf("  %sexport%s [VAR=val..]  Set/show environment variables\n",
           use_color() ? C_GREEN : "", r);
    printf("  %shelp%s          Show this help\n",
           use_color() ? C_GREEN : "", r);
    printf("\n");
    printf("%sFeatures:%s\n", h, r);
    printf("  cmd1 | cmd2       Pipe\n");
    printf("  cmd > file        Redirect stdout (overwrite)\n");
    printf("  cmd >> file       Redirect stdout (append)\n");
    printf("  cmd < file        Redirect stdin\n");
    printf("  cmd &             Background\n");
    printf("  cmd1 ; cmd2       Sequential\n");
    printf("  $VAR, $?          Variable expansion\n");
    printf("  \"...\" '...'       Quoting\n");
    printf("  ~                 Home directory\n");
    printf("  ~/.tinyshellrc    Config file (sourced on startup)\n");
    printf("  Ctrl+C            Interrupt (doesn't kill shell)\n");

    return 0;
}

int builtin_clear(char **args)
{
    (void)args;
    printf("\033[H\033[J");
    return 0;
}

int builtin_echo(char **args)
{
    for (int i = 1; args[i] != NULL; i++)
    {
        printf("%s", args[i]);
        if (args[i + 1] != NULL)
            printf(" ");
    }
    printf("\n");
    return 0;
}

int builtin_export(char **args)
{
    if (args[1] == NULL)
    {
        for (char **e = environ; *e; e++)
            printf("%s\n", *e);
        return 0;
    }

    for (int i = 1; args[i]; i++)
    {
        char *eq = strchr(args[i], '=');
        if (eq)
        {
            *eq = '\0';
            setenv(args[i], eq + 1, 1);
            *eq = '=';
        }
        else
        {
            char *val = getenv(args[i]);
            if (val)
                setenv(args[i], val, 1);
        }
    }
    return 0;
}

int builtin_exit(char **args)
{
    int code = 0;
    if (args[1] != NULL)
        code = atoi(args[1]);
    exit(code);
    return 0;
}