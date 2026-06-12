#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

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

    printf("TinyShell — built-in commands:\n");
    printf("  cd [dir]      Change directory (default: ~, -: previous)\n");
    printf("  pwd           Print working directory\n");
    printf("  echo <text>   Print text\n");
    printf("  clear         Clear screen\n");
    printf("  exit [n]      Exit shell (default: 0)\n");
    printf("  help          Show this help\n");
    printf("\n");
    printf("Features:\n");
    printf("  cmd1 | cmd2       Pipe\n");
    printf("  cmd > file        Redirect stdout (overwrite)\n");
    printf("  cmd >> file       Redirect stdout (append)\n");
    printf("  cmd < file        Redirect stdin\n");
    printf("  cmd &             Background\n");
    printf("  cmd1 ; cmd2       Sequential\n");
    printf("  $VAR              Variable expansion\n");
    printf("  \"...\" '...'       Quoting\n");
    printf("  ~                 Home directory\n");
    printf("  $? in prompt      Exit status\n");

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

int builtin_exit(char **args)
{
    int code = 0;
    if (args[1] != NULL)
        code = atoi(args[1]);
    exit(code);
    return 0;
}