#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "../include/builtins.h"
#include "../include/command.h"
#include "../include/color.h"

typedef struct
{
    char *name;
    int (*func)(char **);
} builtin_t;

static builtin_t builtins[] = {
    {"cd",     builtin_cd},
    {"pwd",    builtin_pwd},
    {"echo",   builtin_echo},
    {"clear",  builtin_clear},
    {"exit",   builtin_exit},
    {"export", builtin_export},
    {"help",   builtin_help},
    {NULL, NULL}
};

static int is_builtin(const char *name)
{
    for (int i = 0; builtins[i].name != NULL; i++)
        if (strcmp(name, builtins[i].name) == 0)
            return 1;
    return 0;
}

static int run_builtin(char **args)
{
    for (int i = 0; builtins[i].name != NULL; i++)
        if (strcmp(args[0], builtins[i].name) == 0)
            return builtins[i].func(args);
    return 1;
}

static int setup_redirects(command_t *cmd)
{
    if (cmd->infile)
    {
        int fd = open(cmd->infile, O_RDONLY);
        if (fd < 0)
        {
            perror(cmd->infile);
            return -1;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (cmd->outfile)
    {
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
        mode_t mode = 0644;
        int fd = open(cmd->outfile, flags, mode);
        if (fd < 0)
        {
            perror(cmd->outfile);
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    return 0;
}

static int exec_command(command_t *cmd)
{
    if (cmd->args == NULL || cmd->args[0] == NULL)
        return 1;

    signal(SIGINT, SIG_DFL);
    setup_redirects(cmd);

    if (is_builtin(cmd->args[0]))
        exit(run_builtin(cmd->args));

    execvp(cmd->args[0], cmd->args);
    fprintf(stderr, "%stinyshell: %s: command not found%s\n",
            use_color() ? C_RED : "", cmd->args[0],
            use_color() ? C_RESET : "");
    exit(127);
}

static int spawn_command(command_t *cmd, int in_fd, int out_fd)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    if (pid == 0)
    {
        if (in_fd != -1)
        {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if (out_fd != -1)
        {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }

        exec_command(cmd);
    }

    return pid;
}

int execute_pipeline(pipeline_t *pl)
{
    int n = pl->ncommands;
    if (n == 0) return 0;

    if (n == 1 && !pl->commands[0].background &&
        !pl->commands[0].infile && !pl->commands[0].outfile &&
        is_builtin(pl->commands[0].args[0]))
    {
        return run_builtin(pl->commands[0].args);
    }

    int in_fd = -1;
    pid_t *children = malloc(n * sizeof(pid_t));
    int nchildren = 0;

    for (int i = 0; i < n; i++)
    {
        int next_pipe[2] = {-1, -1};

        if (i < n - 1)
        {
            if (pipe(next_pipe) < 0)
            {
                perror("pipe");
                break;
            }
        }

        int out_fd = (i < n - 1) ? next_pipe[1] : -1;
        pid_t pid = spawn_command(&pl->commands[i], in_fd, out_fd);

        if (pid > 0)
            children[nchildren++] = pid;

        if (in_fd != -1)
            close(in_fd);

        if (out_fd != -1)
            close(out_fd);

        in_fd = (i < n - 1) ? next_pipe[0] : -1;
    }

    if (in_fd != -1)
        close(in_fd);

    int last_status = 0;

    if (pl->commands[0].background)
    {
        if (nchildren > 0)
            printf("[%d] %d\n", nchildren, children[nchildren - 1]);
    }
    else
    {
        for (int i = 0; i < nchildren; i++)
        {
            int status;
            waitpid(children[i], &status, 0);
            if (i == nchildren - 1)
                last_status = WEXITSTATUS(status);
        }
    }

    free(children);
    return last_status;
}
